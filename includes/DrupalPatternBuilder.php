<?php

/**
 * @file
 * Class to build pattern objects based on entity storage.
 *
 * File is not namespaced in order to work with D7 class loading.
 */

/**
 * Class to build pattern objects based on entity storage.
 */
class DrupalPatternBuilder {
  const SCHEMA_ENTITY_TYPE = 'paragraphs_item';
  const FIELD_DISPLAY_INSTANCE_HANDLER_CLASS_DEFAULT = 'DrupalPatternBuilderDisplayInstance';
  const PB_NATIVE_ENTITY_SCHEMA_NAME = 'pb_entity';
  const PB_NATIVE_RAW_SCHEMA_NAME = 'pb_raw';

  /**
   * The entity object.
   *
   * @var object
   */
  protected $entity;

  /**
   * The entity type.
   *
   * @var string
   */
  protected $entityType;

  /**
   * The entity id.
   *
   * @var integer
   */
  protected $entityId;

  /**
   * The entity revision id.
   *
   * @var integer
   */
  protected $entityRevisionId;

  /**
   * The entity bundle name.
   *
   * @var string
   */
  protected $entityBundle;

  /**
   * The schema controller object.
   *
   * @var DrupalPatternBuilderSchema
   */
  protected $schemaController;

  /**
   * The built component.
   *
   * @var DrupalPatternBuilderComponent
   */
  protected $component;

  /**
   * The built view mode of the current component.
   *
   * @var string
   */
  protected $builtViewMode = '';

  /**
   * Flag set if initialized entity is a schema entity.
   *
   * @var boolean
   */
  protected $isSchemaEntity = FALSE;

  /**
   * Twig environment options.
   *
   * @var array
   *
   * @see http://twig.sensiolabs.org/doc/api.html#basics
   */
  protected $twigEnvOptions = array(
    // Disable TWIG escaping strategy to ensure Drupal input filters control
    // the rendered markup.
    'autoescape' => FALSE,
  );

  /**
   * Constructor.
   *
   * @param string $entity_type
   *   The entity type.
   * @param object $entity
   *   The entity object.
   */
  public function __construct($entity_type, $entity) {
    $this->entityType = $entity_type;
    $this->entity = $entity;
    list($entity_id, $entity_revision_id, $entity_bundle) = entity_extract_ids($entity_type, $entity);

    $this->entityId = $entity_id;
    $this->entityRevisionId = $entity_revision_id;
    $this->entityBundle = $entity_bundle;

    // Initialize the schema library.
    $this->schemaController = patternbuilder_get($this->twigEnvOptions);

    // Create the root component.
    $this->initComponent();
  }

  /**
   * Initialize the root component.
   */
  protected function initComponent() {
    $this->builtViewMode = NULL;
    $component = $this->loadSchemaByEntity($this->entityType, $this->entity);
    if ($component) {
      $this->isSchemaEntity = TRUE;
      $this->component = $component;
    }
    else {
      $this->isSchemaEntity = FALSE;
      $this->component = NULL;
    }
  }

  /**
   * Returns an array of allowed reference entity types.
   *
   * These types determine if the field is recursed into to extract values.
   *
   * @return array
   *   An array of entity types.
   */
  public static function allowedReferenceEntityTypes() {
    return array('field_collection_item', 'paragraphs_item');
  }

  /**
   * Returns an array of property names that contain the schema name.
   *
   * @return array
   *   An array of property names.
   */
  public static function schemaPropertyNames() {
    return array('name');
  }

  /**
   * Determines if conditions are met to build the object.
   *
   * @return bool
   *   TRUE if object can be built.
   */
  public function canBuild() {
    return $this->isSchemaEntity;
  }

  /**
   * Loads an schema object based on the Drupal entity.
   *
   * @param string $entity_type
   *   The Drupal entity type.
   * @param object $entity
   *   The Drupal entity object.
   * @param string $component_class
   *   Type of class to be returned on successful object creation.
   *
   * @return object|null
   *   Either an Component object or NULL.
   */
  protected function loadSchemaByEntity($entity_type, $entity, $component_class = 'DrupalPatternBuilderComponent') {
    list($entity_id, $entity_vid, $entity_bundle) = entity_extract_ids($entity_type, $entity);
    if (!$entity_bundle) {
      return NULL;
    }

    $schema_name = NULL;

    // Reference entities - field collections, paragraphs.
    if ($entity_type == static::SCHEMA_ENTITY_TYPE || in_array($entity_type, static::allowedReferenceEntityTypes())) {
      $schema_name = $this->findSchemaByEntityField($entity_type, $entity);
    }

    // Schema entity fallback if no schema name stored.
    if (!isset($schema_name)) {
      $schema_name = $this->findSchemaByEntityBundleMap($entity_type, $entity);
    }

    if ($schema_name) {
      $component = $this->schemaController->load($schema_name, $component_class);
      if ($component) {
        if ($component instanceof DrupalPatternBuilderComponent) {
          $component->source($entity, $entity_type);
        }

        return $component;
      }
    }
  }

  /**
   * Loads an schema object based on the Drupal entity field values.
   *
   * @param string $entity_type
   *   The Drupal entity type.
   * @param object $entity
   *   The Drupal entity object.
   *
   * @return string|null
   *   Either the schema name or NULL.
   */
  protected function findSchemaByEntityField($entity_type, $entity) {
    $schema_props = static::schemaPropertyNames();
    if (empty($schema_props)) {
      return NULL;
    }

    list($entity_id, $entity_vid, $entity_bundle) = entity_extract_ids($entity_type, $entity);
    if (!$entity_bundle) {
      return NULL;
    }

    $field_instances = _patternbuilder_field_info_instances($entity_type, $entity_bundle);
    foreach ($schema_props as $schema_prop) {
      foreach ($field_instances as $field_name => $field_instance) {
        $pb_instance_settings = $field_instance['settings']['patternbuilder'];

        // Skip if not a match.
        if (!isset($pb_instance_settings['real_property_name']) || $pb_instance_settings['real_property_name'] != $schema_prop) {
          continue;
        }

        // Force default for readonly fields.
        if (!empty($pb_instance_settings['readonly']) && !empty($field_instance['default_value'])) {
          $field_default = reset($field_instance['default_value']);
          // Assume the value key is the first value.
          $field_default_value = reset($field_default);
          if ($field_default_value) {
            return $field_default_value;
          }
        }

        // Check field values.
        $field_items = static::fieldGetItems($entity_type, $entity, $field_name);
        if (!empty($field_items)) {
          foreach ($field_items as $field_delta => $field_item) {
            // Assume value key is the first value.
            if ($field_item && ($field_value = reset($field_item))) {
              return $field_value;
            }
          }
        }
      }
    }
  }

  /**
   * Loads an schema object based on the Drupal entity bundle to schema map.
   *
   * @param string $entity_type
   *   The Drupal entity type.
   * @param object $entity
   *   The Drupal entity object.
   *
   * @return string|null
   *   Either the schema name or NULL.
   */
  protected function findSchemaByEntityBundleMap($entity_type, $entity) {
    if ($entity_type !== static::SCHEMA_ENTITY_TYPE) {
      return NULL;
    }

    list($entity_id, $entity_vid, $entity_bundle) = entity_extract_ids($entity_type, $entity);
    if (!$entity_bundle) {
      return NULL;
    }

    $schema_relation = patternbuilder_get_bundle_component($entity_bundle);
    return !empty($schema_relation->machine_name) ? $schema_relation->machine_name : NULL;
  }

  /**
   * Renders the schema object.
   *
   * @param string $view_mode
   *   The Drupal field display mode.
   *
   * @return string
   *   Rendered markup.
   */
  public function render($view_mode = 'full') {
    if (empty($this->builtViewMode)) {
      // First time building.
      $this->build($this->component, $this->entityType, $this->entity, $view_mode);
    }
    elseif ($this->builtViewMode !== $view_mode) {
      // Reinitialize the component when this builder is re-used for different
      // view mode.
      $this->initComponent();
      $this->build($this->component, $this->entityType, $this->entity, $view_mode);
    }

    $this->setComponentMetaData();
    return $this->component->render();
  }

  /**
   * Set component metadata for the schema entity.
   */
  protected function setComponentMetaData() {
    if ($this->isSchemaEntity && $this->component) {
      $data = $this->createMetaData();
      if ($data) {
        static::setComponentValue($this->component, 'meta', $data);
      }
    }
  }

  /**
   * Create the metadata for the schema entity.
   *
   * @return array
   *   An array of meta data.
   */
  protected function createMetaData() {
    $meta = array();
    if ($this->entityId) {
      if ($this->entityType == static::SCHEMA_ENTITY_TYPE) {
        // Set simple id for schemas.
        $meta['uniqueId'] = (string) $this->entityId;
      }
      else {
        // Set id for non-schema entities to avoid collisions.
        // This is an edge case since the field formatter is only for
        // paragraph fields. However, this is a class and can be used outside
        // of the field formatter.
        $meta['uniqueId'] = drupal_html_class($this->entityType . '-' . $this->entityId);
      }
    }

    return $meta;
  }

  /**
   * Builds property values based on the schema entity.
   *
   * @param mixed $component
   *   The component object or array for tuples.
   * @param string $entity_type
   *   The entity type.
   * @param object $entity
   *   The entity object.
   * @param string $view_mode
   *   The entity display view mode.
   * @param string $field_name
   *   The field name of the specific field to build.
   *
   * @return DrupalPatternBuilder
   *   Returns the current instance of this class.
   */
  protected function build(&$component, $entity_type, $entity, $view_mode = 'full', $field_name = NULL) {
    // Exit if built already OR cannot build.
    if (!empty($this->builtViewMode) || !$this->canBuild()) {
      return $this;
    }

    // Skip invalid entity types.
    $entity_info = entity_get_info($entity_type);
    if (empty($entity_info)) {
      return $this;
    }

    list($entity_id, $entity_revision_id, $entity_bundle) = entity_extract_ids($entity_type, $entity);
    $entity_wrapper = entity_metadata_wrapper($entity_type, $entity);

    // Allowed references.
    $ref_entity_types = static::allowedReferenceEntityTypes();

    if (isset($field_name)) {
      // Process fields.
      $field_instance = field_info_instance($entity_type, $field_name, $entity_bundle);
      $display = static::createDisplayHandler($entity, $field_instance, $view_mode);

      if (isset($entity_wrapper->{$field_name}) && $display->canRender()) {
        $field_display = $display->getDisplay();
        $field_items = static::fieldGetItems($entity_type, $entity, $field_name);

        if ($field_items) {
          $field_data_type_defined = $entity_wrapper->{$field_name}->type();
          $field_data_type = entity_property_extract_innermost_type($field_data_type_defined);
          $field_entity_info = entity_get_info($field_data_type);
          $field_is_chainable_reference = $field_entity_info && in_array($field_data_type, $ref_entity_types, TRUE);
          $field_view_mode = isset($field_display['settings']['view_mode']) ? $field_display['settings']['view_mode'] : 'default';

          if ($field_entity_info) {
            // Entity references.
            $field_has_wrapped_schemas = FALSE;
            foreach ($field_items as $field_delta => $field_item) {
              $ref_entity_type = $field_data_type;
              $ref_entity = static::loadReferenceItemEntity($ref_entity_type, $field_item);

              if ($ref_entity && entity_access('view', $ref_entity_type, $ref_entity)) {
                // If not chainable, then determine if this is a wrapped schema.
                if (!$field_is_chainable_reference) {
                  // Attempt to unwrap the schema.
                  $field_wrapped_schema_instance = patternbuilder_wrapped_schema_field_instance_by_entity($ref_entity_type, $ref_entity);
                  if (!empty($field_wrapped_schema_instance['field_name'])) {
                    $wrapped_field_items = static::fieldGetItems($ref_entity_type, $ref_entity, $field_wrapped_schema_instance['field_name']);
                    if (!empty($wrapped_field_items)) {
                      // There can be only 1 wrapped field item.
                      $wrapped_field_item = reset($wrapped_field_items);
                      $wrapped_ref_entity_type = static::SCHEMA_ENTITY_TYPE;
                      $wrapped_ref_entity = static::loadReferenceItemEntity($wrapped_ref_entity_type, $wrapped_field_item);
                      if ($wrapped_ref_entity && entity_access('view', $wrapped_ref_entity_type, $wrapped_ref_entity)) {
                        $ref_entity_type = $wrapped_ref_entity_type;
                        $ref_entity = $wrapped_ref_entity;
                      }
                    }
                  }
                }

                // Load the pattern component.
                $ref_component = $this->loadSchemaByEntity($ref_entity_type, $ref_entity);

                // If not a patttern component.
                if (empty($ref_component)) {
                  if ($ref_entity_type == static::SCHEMA_ENTITY_TYPE) {
                    // Render content for non-pattern schema entity types.
                    $ref_component = $this->createNonSchemaEntityComponent($ref_entity_type, $ref_entity, $field_view_mode);
                  }
                  elseif (_patternbuilder_entity_is_tuple($ref_entity_type, $ref_entity)) {
                    // Tuples are build as an array of items.
                    // @todo: Should this be a custom value array component to
                    // support tuples of tuples?
                    $ref_component = array();
                  }
                  elseif ($field_is_chainable_reference) {
                    // Fallback to a generic value component.
                    $ref_component = new DrupalPatternBuilderValueProperty();
                  }
                }

                if (isset($ref_component)) {
                  // Pattern components.
                  $this->build($ref_component, $ref_entity_type, $ref_entity, $field_view_mode);
                  static::setComponentValue($component, $display->getPbSettings('real_property_name'), $ref_component, $display->getPbSettings('parent_property_names_array'));
                }
                else {
                  // Set rendered entity reference fields as raw components.
                  $this->fieldSetComponent($component, $display, TRUE);
                }
              }
            }
          }
          else {
            // Rendered field.
            $this->fieldSetComponent($component, $display);
          }
        }
      }
    }
    else {
      // Process entity.
      $field_instances = _patternbuilder_field_info_instances($entity_type, $entity_bundle);
      if ($field_instances) {
        foreach ($field_instances as $field_name => $field_instance) {
          $this->build($component, $entity_type, $entity, $view_mode, $field_name);
        }
      }

      // Set flag when the root entity is built.
      if ($this->entityId && $entity_id == $this->entityId && $this->entityRevisionId && $entity_revision_id == $this->entityRevisionId) {
        $this->builtViewMode = $view_mode;
      }

      // Clear all field prepared flags.
      static::clearFieldViewPreparedFlags($entity);
    }

    return $this;
  }

  /**
   * Create a component for a non-schema driven entity.
   *
   * This renders the entity and then uses Pattern Builders native value
   * component. The name is set to "pb_raw" so that "pb_raw.twig" can be
   * used to render the component.
   * To override "pb_raw.twig", create a custom "pb_raw.twig" and remove
   * the patternbuilder template directory at
   * "admin/config/content/patternbuilder".
   *
   * @param string $entity_type
   *   The entity type.
   * @param string $entity
   *   The entity object.
   * @param string $view_mode
   *   The Drupal field display mode.
   *
   * @return Component|null
   *   The schema component object.
   */
  protected function createNonSchemaEntityComponent($entity_type, $entity, $view_mode = 'full') {
    $name = static::PB_NATIVE_ENTITY_SCHEMA_NAME;
    if (empty($name)) {
      return NULL;
    }

    // Check view access.
    if (!entity_access('view', $entity_type, $entity)) {
      return NULL;
    }

    // Build component.
    $component = NULL;
    $render = array();
    if (method_exists($entity, 'view')) {
      $render = $entity->view($view_mode);
    }
    else {
      list($entity_id) = entity_extract_ids($entity_type, $entity);
      $entity_id = $entity_id ? $entity_id : 0;
      $render = entity_view($entity_type, array($entity_id => $entity), $view_mode);
    }

    if (!empty($render)) {
      $rendered = render($render);
      if ($rendered) {
        $component = new DrupalPatternBuilderValueProperty();
        $component->setByAssoc(array(
          'name' => $name,
          'content' => $rendered,
          'classes_array' => array(
            drupal_html_class($name . '-' . $entity_type),
            drupal_html_class($name . '-' . $entity_type . '-' . $view_mode),
          ),
        ));
      }
    }

    return $component;
  }

  /**
   * Create a component for a rendered markup.
   *
   * @param string $markup
   *   The rendered content.
   *
   * @return Component|null
   *   The schema component object.
   */
  protected function createRawComponent($markup) {
    $name = static::PB_NATIVE_RAW_SCHEMA_NAME;
    if (empty($name)) {
      return NULL;
    }

    // Build component.
    $component = new DrupalPatternBuilderValueProperty();
    $component->setByAssoc(array(
      'name' => $name,
      'content' => $markup,
    ));

    return $component;
  }

  /**
   * Maps rendered field items to the component property.
   *
   * @param Component|array $component
   *   The schema component object.
   * @param DrupalPatternBuilderDisplayInstance $display
   *   An instance of a display handler.
   * @param bool $create_item_components
   *   Create raw value components for each field item.
   */
  protected function fieldSetComponent($component, DrupalPatternBuilderDisplayInstance $display, $create_item_components = FALSE) {
    if ($values = $display->view()) {
      $property_name = $display->getPbSettings('real_property_name');
      $parent_property_names = $display->getPbSettings('parent_property_names_array');
      if ($create_item_components) {
        foreach ($values as $delta => $value) {
          $item_component = $this->createRawComponent($value);
          static::setComponentValue($component, $property_name, $item_component, $parent_property_names);
        }
      }
      else {
        foreach ($values as $delta => $value) {
          static::setComponentValue($component, $property_name, $value, $parent_property_names);
        }
      }
    }
  }

  /**
   * Returns the map of field type to display handler class name.
   *
   * @return array
   *   The map array.
   */
  public static function fieldDisplayInstanceHandlerTypeMap() {
    return array(
      'list_boolean' => 'DrupalPatternBuilderDisplayInstanceBoolean',
    );
  }

  /**
   * Determines the class name of the display handler.
   *
   * @param array $field_instance
   *   The field instance info.
   *
   * @return string
   *   The display handler class name.
   */
  protected static function fieldDisplayInstanceHandlerClass(array $field_instance) {
    $class_name = static::FIELD_DISPLAY_INSTANCE_HANDLER_CLASS_DEFAULT;
    $class_map = static::fieldDisplayInstanceHandlerTypeMap();
    if ($class_map && isset($field_instance['field_name']) && ($field = field_info_field($field_instance['field_name'])) && !empty($field['type'])) {
      if (isset($class_map[$field['type']])) {
        $class_name = $class_map[$field['type']];
      }
    }

    return $class_name;
  }

  /**
   * Creates an instance of the display handler.
   *
   * @param object $entity
   *   The entity object.
   * @param array $field_instance
   *   The field instance info.
   * @param mixed $field_display
   *   Can be either the name of a view mode, or an array of display settings.
   *   See field_view_field() for more information.
   * @param string $langcode
   *   The language the field values are to be shown in.
   *
   * @return object|null
   *   An instance of the display handler.
   */
  protected static function createDisplayHandler($entity, array $field_instance, $field_display = 'default', $langcode = NULL) {
    $class_name = static::fieldDisplayInstanceHandlerClass($field_instance);
    if ($class_name) {
      return new $class_name($entity, $field_instance, $field_display, $langcode);
    }
  }

  /**
   * Clears all field view prepared flags stored on the entity object.
   *
   * @param object $entity
   *   The entity object.
   */
  protected static function clearFieldViewPreparedFlags($entity) {
    unset($entity->_field_view_prepared);
    unset($entity->_pb_field_views_prepared);
  }

  /**
   * Set a component property value.
   *
   * @param Component|array $component
   *   The schema component object.
   * @param string $property_name
   *   A single property name.
   * @param mixed $value
   *   The property value.
   * @param array $parent_property_names
   *   An array of parent property names in hierarchical order.
   * @param bool $append
   *   TRUE to append values.
   */
  protected static function setComponentValue(&$component, $property_name, $value, array $parent_property_names = array(), $append = TRUE) {
    // Process nested property.
    if (!empty($parent_property_names)) {
      // Get top level name to set.
      $set_name = array_shift($parent_property_names);

      // Append property name to the parents.
      $parent_property_names[] = $property_name;

      // Build values array.
      $set_value = array();
      drupal_array_set_nested_value($set_value, $parent_property_names, $value);

      return static::setComponentValue($component, $set_name, $set_value, array(), $append);
    }

    // Set the value on the component.
    if (is_array($component)) {
      if ($append) {
        $component[] = $value;
      }
      else {
        $component = $value;
      }
    }
    elseif (method_exists($component, 'set')) {
      $component->set($property_name, $value);
    }
  }

  /**
   * Returns normalized keys for an entity reference field item.
   *
   * @param string $entity_type
   *   The entity type of the referenced entity.
   * @param array $item
   *   The field item array.
   *
   * @return array
   *   An array with keys of 'id', 'revision_id', and 'entity'.
   */
  protected static function getReferenceItemEntityValues($entity_type, array $item) {
    $return = array('id' => NULL, 'revision_id' => NULL, 'entity' => NULL);
    $info = entity_get_info($entity_type);

    if (isset($info['entity keys']['id']) && $info['entity keys']['id'] && isset($item[$info['entity keys']['id']])) {
      $return['id'] = $item[$info['entity keys']['id']];
    }
    elseif (isset($item['value'])) {
      $return['id'] = $item['value'];
    }

    if (isset($info['entity keys']['revision']) && $info['entity keys']['revision'] && isset($item[$info['entity keys']['revision']])) {
      $return['revision_id'] = $item[$info['entity keys']['revision']];
    }
    elseif (isset($item['revision_id'])) {
      $return['revision_id'] = $item['revision_id'];
    }

    if (isset($item['entity'])) {
      $return['entity'] = $item['entity'];
    }

    return $return;
  }

  /**
   * Loads the entity for reference field item.
   *
   * @param string $entity_type
   *   The entity type of the referenced entity.
   * @param array $item
   *   The field item array.
   * @param bool $skip_existing_entity
   *   TRUE to skip any existing entity set in the item's "entity" key.
   *
   * @return object|null
   *   The entity object.
   */
  protected static function loadReferenceItemEntity($entity_type, array $item, $skip_existing_entity = FALSE) {
    $values = static::getReferenceItemEntityValues($entity_type, $item);

    if (!$skip_existing_entity && !empty($values['entity'])) {
      return $values['entity'];
    }
    elseif (!empty($values['revision_id'])) {
      return entity_revision_load($entity_type, $values['revision_id']);
    }
    elseif (!empty($values['id'])) {
      return entity_load_single($entity_type, $values['id']);
    }
    elseif (!empty($values['vid'])) {
      return entity_revision_load($entity_type, $values['vid']);
    }
    elseif (!empty($values['nid'])) {
      return entity_load_single($entity_type, $values['nid']);
    }
  }

  /**
   * Helper function to overcome caveats of field_language().
   *
   * The function field_language() has a static cache indexed by entity type
   * and entity id. This causes a collision when multiple new entities of the
   * same entity type are rendered.
   *
   * @param string $entity_type
   *   The entity type.
   * @param object $entity
   *   The entity object.
   * @param string $field_name
   *   The field name.
   * @param string $langcode
   *   Optional. The language code.
   *
   * @return array
   *   The items array.
   */
  public static function fieldGetItems($entity_type, $entity, $field_name, $langcode = NULL) {
    $langcode = static::fieldLanguage($entity_type, $entity, $field_name, $langcode);
    return isset($entity->{$field_name}[$langcode]) ? $entity->{$field_name}[$langcode] : FALSE;
  }

  /**
   * Helper function to overcome caveats of field_language().
   *
   * The function field_language() has a static cache indexed by entity type
   * and entity id. This causes a collision when multiple new entities of the
   * same entity type and different bundle are rendered.
   *
   * Drupal core issue: https://www.drupal.org/node/2201251
   * This function overcomes this issue by generating fake ids. This reduces
   * the need for a core patch to utilize patternbuilder with the
   * paragraphs_previewer and patternbuilder_previewer module.
   *
   * @param string $entity_type
   *   The entity type.
   * @param object $entity
   *   The entity object.
   * @param string $field_name
   *   The field name.
   * @param string $langcode
   *   Optional. The language code.
   *
   * @return string
   *   The field language code.
   */
  public static function fieldLanguage($entity_type, $entity, $field_name, $langcode = NULL) {
    // Store fake ids for new entities to avoid field_language() cache issue.
    static $drupal_static_fast;
    if (!isset($drupal_static_fast)) {
      $drupal_static_fast['id'] = &drupal_static('DrupalPatternBuilder::' . __FUNCTION__);
    }
    $fake_id = &$drupal_static_fast['id'];

    $info = entity_get_info($entity_type);
    if (empty($entity->{$info['entity keys']['id']})) {
      // Generate a fake id to bypass the field_language cache.
      $fake_id++;
      $id = 'pb' . $fake_id;
      $entity->{$info['entity keys']['id']} = $id;
      $langcode = field_language($entity_type, $entity, $field_name, $langcode);
      $field_language_cache = &drupal_static('field_language', array());
      unset($field_language_cache[$entity_type][$id]);
      $entity->{$info['entity keys']['id']} = NULL;
    }
    else {
      $langcode = field_language($entity_type, $entity, $field_name, $langcode);
    }

    return $langcode;
  }

}
