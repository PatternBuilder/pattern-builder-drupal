<?php

/**
 * @file
 * Class to handle field display instances.
 *
 * File is not namespaced in order to work with D7 class loading.
 */

/**
 * Class to handle field display instances.
 */
class DrupalPatternBuilderDisplayInstance {

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
   * The field display language code.
   *
   * @var string
   */
  protected $language;

  /**
   * The field type.
   *
   * @var string
   */
  protected $fieldType;

  /**
   * The field instance info.
   *
   * @var array
   */
  protected $instance;

  /**
   * The field instance patternbuilder settings.
   *
   * @var array
   */
  protected $pbSettings;

  /**
   * The field instance patternbuilder property map.
   *
   * @var array
   */
  protected $propertyMap = array();

  /**
   * The field display info.
   *
   * @var array
   */
  protected $display;

  /**
   * The field display view mode.
   *
   * @var array
   */
  protected $viewMode;

  /**
   * Flag set once the display has been prepared.
   *
   * @var bool
   */
  protected $prepared = FALSE;

  /**
   * Constructor.
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
   */
  public function __construct($entity, array $field_instance, $field_display = 'default', $langcode = NULL) {
    if (empty($field_instance['entity_type']) || empty($field_instance['field_name'])) {
      return;
    }

    $field = field_info_field($field_instance['field_name']);
    if (empty($field['type'])) {
      return;
    }

    $this->entityType = $field_instance['entity_type'];
    $this->entity = $entity;
    list($entity_id, $entity_revision_id, $entity_bundle) = entity_extract_ids($this->entityType, $this->entity);

    $this->entityId = $entity_id;
    $this->entityRevisionId = $entity_revision_id;
    $this->entityBundle = $entity_bundle;

    $this->fieldName = $field_instance['field_name'];
    $this->fieldType = $field['type'];
    $this->instance = $field_instance;
    $this->display = $field_display;
    $this->language = DrupalPatternBuilder::fieldLanguage($this->entityType, $this->entity, $this->instance['field_name'], $langcode);

    $this->prepare();
  }

  /**
   * Retrieves the field type.
   */
  public function getFieldType() {
    return $this->fieldType;
  }

  /**
   * Retrieves patternbuilder settings.
   *
   * @param string $name
   *   The setting key name.
   *
   * @return mixed
   *   The setting if $name is provided, else all settings.
   */
  public function getPbSettings($name = NULL) {
    if (isset($name)) {
      return isset($this->pbSettings[$name]) ? $this->pbSettings[$name] : NULL;
    }

    return $this->pbSettings;
  }

  /**
   * Retrieves field instance info.
   *
   * @param string $name
   *   The instance info key name.
   *
   * @return mixed
   *   The setting if $name is provided, else all info.
   */
  public function getInstance($name = NULL) {
    if (isset($name)) {
      return isset($this->instance[$name]) ? $this->instance[$name] : NULL;
    }

    return $this->instance;
  }

  /**
   * Retrieves field display settings.
   *
   * @param string $name
   *   The setting key name.
   *
   * @return mixed
   *   The setting if $name is provided, else all settings.
   */
  public function getDisplay($name = NULL) {
    if (isset($name)) {
      return isset($this->display[$name]) ? $this->display[$name] : NULL;
    }

    return $this->display;
  }

  /**
   * Determines if the field display is shown.
   *
   * @return bool
   *   TRUE if the display is hidden.
   */
  public function isShown() {
    return isset($this->display) && $this->getDisplay('type') !== 'hidden' && $this->getPbSettings('real_property_name');
  }

  /**
   * Determines if the field display is hidden.
   *
   * @return bool
   *   TRUE if the display is hidden.
   */
  public function isHidden() {
    return !$this->isShown();
  }

  /**
   * Determines if the field is readonly.
   *
   * @return bool
   *   TRUE if the display is hidden.
   */
  public function isReadonly() {
    return $this->getPbSettings('readonly');
  }

  /**
   * Determines if the field display can be rendered.
   *
   * @return bool
   *   TRUE if the display is hidden.
   */
  public function canRender() {
    return !$this->isReadonly() && $this->isShown();
  }

  /**
   * Retrieves field display view mode.
   *
   * @return string|null
   *   The setting if $name is provided, else all settings.
   */
  public function getViewMode() {
    return $this->viewMode;
  }

  /**
   * Retrieves the property map.
   *
   * @param string $name
   *   The Drupal field property name.
   *
   * @return mixed
   *   The schema property name if $name is provided, else all mappings.
   */
  public function getPropertyMap($name) {
    if (isset($name)) {
      return isset($this->propertyMap[$name]) ? $this->propertyMap[$name] : NULL;
    }

    return $this->propertyMap;
  }

  /**
   * Retrieves the entity object.
   */
  public function getEntity() {
    return $this->entity;
  }

  /**
   * Retrieves the entity object.
   */
  public function getEntityIds() {
    return array($this->entityId, $this->entityRevisionId, $this->entityBundle);
  }

  /**
   * Render the field items.
   *
   * @return array
   *   An array of rendered field items.
   */
  public function view() {
    if (!$this->prepared) {
      if (!$this->prepare()->prepared) {
        return array();
      }
    }

    if (!empty($this->propertyMap)) {
      // Property map processing.
      $values = $this->viewPropertyMap();
    }
    else {
      // Normal field rendering.
      $values = $this->viewField();
    }

    return $values;
  }

  /**
   * Renders each field item separately to avoid wrapping markup.
   *
   * @return array
   *   An array of rendered field items.
   */
  protected function viewField() {
    $renders = array();
    $field_name = $this->instance['field_name'];

    // Process items.
    // Note: Using field_view_field() avoid issue with field_view_value()
    // not finding the correct language due to field_language() static cache
    // indexed by entity type and id which fails for multiple new entities of
    // the same entity type that do not have an id.
    $item_renders = field_view_field($this->entityType, $this->entity, $field_name, $this->display, $this->language);
    if ($item_renders && ($deltas = element_children($item_renders))) {
      foreach ($deltas as $delta) {
        if (is_numeric($delta)) {
          // Propogate #access to each item.
          // @see field_view_value()
          if (isset($item_renders['#access'])) {
            $item_renders[$delta]['#access'] = $item_renders['#access'];
          }

          $item_rendered = render($item_renders[$delta]);
          if ($item_rendered) {
            $renders[$delta] = $item_rendered;
          }
        }
      }
    }

    return $renders;
  }

  /**
   * Creates value objects for each field item via the property map.
   *
   * @return array
   *   An array of rendered field items.
   */
  protected function viewPropertyMap() {
    if (empty($this->propertyMap)) {
      return array();
    }

    $field_name = $this->instance['field_name'];
    $items = DrupalPatternBuilder::fieldGetItems($this->entityType, $this->entity, $field_name, $this->language);
    if (empty($items)) {
      return array();
    }

    // Render field for special property.
    $rendered_prop_name = PATTERNBUILDER_PROPERTY_MAP_RENDERED_NAME;
    $items_rendered = array();
    if (isset($this->propertyMap[$rendered_prop_name])) {
      $items_rendered = $this->viewField();
    }

    // Process items.
    $return = array();
    $chain_delimiter = PATTERNBUILDER_PROPERTY_MAP_CHAIN_DELIMITER;
    foreach ($items as $delta => $item) {
      $prop_values = array();
      foreach ($this->propertyMap as $field_prop => $schema_prop) {
        $prop_value = NULL;
        $sanitize = !$this->fieldPropertyIsSanitized($field_prop);

        if ($field_prop === $rendered_prop_name) {
          if (isset($items_rendered[$delta])) {
            $prop_value = $items_rendered[$delta];
          }
        }
        elseif ($field_prop === 'url') {
          if ($url = $this->fieldGetItemUrl($item)) {
            // Url handling per field type.
            $prop_value = $url;
          }
          elseif (isset($item[$field_prop])) {
            // Direct item 'url' property.
            $prop_value = url($item[$field_prop]);
          }
        }
        elseif (isset($item[$field_prop])) {
          // Direct properties.
          $prop_value = $item[$field_prop];
          if ($sanitize) {
            $prop_value = check_plain($prop_value);
          }
        }
        elseif (strpos($field_prop, $chain_delimiter) !== FALSE) {
          // Chained properties, ex. "attributes:target".
          $field_prop_parts = explode($chain_delimiter, $field_prop);
          $field_prop_key_exists = NULL;
          $field_value = drupal_array_get_nested_value($item, $field_prop_parts, $field_prop_key_exists);
          if ($field_prop_key_exists) {
            $prop_value = $field_value;
            if ($sanitize) {
              $prop_value = check_plain($prop_value);
            }
          }
        }

        // Set schema property value.
        if (isset($prop_value)) {
          $prop_values[$schema_prop] = $prop_value;
        }
      }

      if ($prop_values) {
        $return[$delta] = $prop_values;
      }
    }

    return $return;
  }

  /**
   * Create an id used to flag this field display as prepared.
   *
   * @return string
   *   An id to flag as prepared.
   */
  protected function getPreparedId() {
    $field_name = $this->getInstance('field_name');

    // The hook_field_prepare_view() is called for the field types module.
    $module = $this->getDisplay('module');

    // The formatter type could cause the field type to prepare differently.
    $type = $this->getDisplay('type');

    // Build the id.
    $id = $field_name;
    $id .= '::' . ($module ? $module : '-any');
    $id .= '::' . ($type ? $type : '-any');

    return $id;
  }

  /**
   * Prepare the field display.
   */
  protected function prepare() {
    $field_name = $this->getInstance('field_name');
    if (empty($field_name)) {
      return $this;
    }

    $entity_type = $this->entityType;

    if (empty($this->prepared)) {
      $this->prepared = TRUE;

      // Patternbuilder settings.
      $this->pbSettings = _patternbuilder_field_instance_settings($this->instance);

      // Set property map.
      if (!empty($this->pbSettings['property_map_array'])) {
        $this->propertyMap = $this->pbSettings['property_map_array'];
      }

      // Display settings.
      if (is_array($this->display)) {
        if (empty($this->display['_prepared'])) {
          // When using custom display settings, fill in default values.
          if (empty($this->display['type']) || empty($this->display['module'])) {
            $field_info = field_info_field($field_name);
            $field_cache = _field_info_field_cache();
            $this->display = $field_cache->prepareInstanceDisplay($this->display, $field_info['type']);
            $this->display['_prepared'] = TRUE;
          }
        }
      }
      elseif (is_string($this->display)) {
        $this->viewMode = $this->display;
        $this->display = field_get_display($this->instance, $this->viewMode, $this->entity);
        $field_info = field_info_field($field_name);
        $field_cache = _field_info_field_cache();
        $this->display = $field_cache->prepareInstanceDisplay($this->display, $field_info['type']);
        $this->display['_prepared'] = TRUE;
      }
    }

    // Invoke prepare_view steps if needed.
    // This is needed for fields to prepare and sanitize the values, ex link.
    // See field_view_field().
    $prepared_id = $this->getPreparedId();
    if (empty($this->entity->_field_view_prepared) || (isset($this->entity->_pb_field_views_prepared) && !in_array($prepared_id, $this->entity->_pb_field_views_prepared))) {
      if (!empty($this->entity->{$field_name}[$this->language])) {
        $null = NULL;

        // Single field prepare.
        $options = array(
          'field_name' => $field_name,
          'language' => $this->language,
        );

        // First let the field types do their preparation.
        _field_invoke_multiple('prepare_view', $entity_type, array($this->entityId => $this->entity), $this->display, $null, $options);
        // Then let the formatters do their own specific massaging.
        _field_invoke_multiple_default('prepare_view', $entity_type, array($this->entityId => $this->entity), $this->display, $null, $options);

        // Mark this item as prepared similar to field_view_field.
        // Note: This function and field_view_field only prepare a single field
        // so this flag is meaningless.
        $this->entity->_field_view_prepared = TRUE;
      }

      // Set a specific flag per field display.
      $this->entity->_pb_field_views_prepared[] = $prepared_id;
    }

    return $this;
  }

  /**
   * Determines if this property is sanitized for this field instance.
   *
   * @param string $property_name
   *   The field property name.
   *
   * @return bool
   *   TRUE if the property is sanitized.
   */
  protected function fieldPropertyIsSanitized($property_name) {
    // Link field already sanitizes it data in link_field_prepare_view().
    if ($this->fieldType == 'link_field') {
      return TRUE;
    }

    return $property_name != 'safe_value' && $property_name != PATTERNBUILDER_PROPERTY_MAP_RENDERED_NAME;
  }

  /**
   * Returns a url for the given field item.
   *
   * @param array $item
   *   A field item array.
   *
   * @return string|null
   *   A url.
   */
  protected function fieldGetItemUrl(array $item) {
    $url = NULL;

    switch ($this->fieldType) {
      case 'link_field':
        if (isset($item['url'])) {
          $url_options = $item;
          unset($url_options['title']);
          unset($url_options['url']);
          $url = url($item['url'], $url_options);
        }
        break;

      case 'image':
        if (isset($item['uri'])) {
          $display = $this->getDisplay();
          if (!empty($display['settings']['image_style'])) {
            $url = image_style_url($display['settings']['image_style'], $item['uri']);
          }
          else {
            $url = file_create_url($item['uri']);
          }
        }
        break;

      case 'file':
        if (isset($item['uri'])) {
          $url = file_create_url($item['uri']);
        }
        break;
    }

    return $url;
  }

}
