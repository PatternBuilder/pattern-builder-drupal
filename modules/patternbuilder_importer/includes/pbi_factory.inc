<?php

/**
 * @file
 * Class for importing patterns.
 */

/**
 * Base factory class.
 */
class pbi_factory {
  protected $id;
  protected $bundleName;
  protected $weight;
  protected $entityType = 'paragraphs_item';

  /**
   * Class Construct.
   *
   * @param int|string $id
   *   ID for the pattern.
   * @param string $bundle_name
   *   The machine name of the bundle.
   * @param int $weight
   *   Weight for field ordering.
   * @param string|null $entity_type
   *   The entity type. If not provided, then the default entity type is used.
   */
  public function __construct($id, $bundle_name, &$weight, $entity_type = NULL) {
    $this->id = $id;
    $this->bundleName = $bundle_name;
    $this->weight = $weight;

    if (isset($entity_type)) {
      $this->entityType = $entity_type;
    }
  }

  /**
   * Loops through the properties and creates/calls the correct object.
   *
   * @param object $properties
   *   The properties from the schema.
   * @param mixed $required_properties
   *   An array of required field names else NULL.
   *
   * @return array
   *   An array of field instances created.
   */
  public function process_properties($properties, $required_properties = array()) {
    if (!isset($properties)) {
      return array();
    }

    // Ensure an array for required properties.
    if (empty($required_properties)) {
      $required_properties = array();
    }
    elseif (!is_array($required_properties)) {
      $required_properties = array($required_properties);
    }

    $all_instances_created = array();
    foreach ($properties as $name => &$property) {
      // Skip if non-imported properties.
      if (!pbi_utility::property_is_imported($property)) {
        continue;
      }

      $handler = NULL;
      $new_instances = NULL;
      $meta_type = $this->determine_meta_type($property);

      if ($meta_type) {
        if ($meta_type === '$ref') {
          // Reference resolution.
          $meta_type = NULL;
          $ref_handlers = array(
            'field_ref' => 'pbi_field_ref',
            'field_group_ref' => 'pbi_field_group_ref',
          );
          foreach ($ref_handlers as $ref_meta_type => $ref_handler_class) {
            $ref = new $ref_handler_class($this->id, $this->bundleName, $this->entityType);
            if ($ref_handler_class::dependencies_met() && $ref->reference_exists($property)) {
              $meta_type = $ref_meta_type;
              $new_instances = $ref->save($name, $property, $required_properties, $this->weight);
              break;
            }

            $ref = NULL;
          }

          if (empty($meta_type)) {
            watchdog('patternbuilder_importer', 'Meta type not found for reference "%property".', array(
              '%property' => $name,
            ));
          }
        }
        elseif ($meta_type && ($handler_class = static::get_meta_type_handler($meta_type)) && $handler_class::dependencies_met()) {
          // Field handlers.
          $handler = new $handler_class($this->id, $this->bundleName, $this->entityType);
          if (!($handler instanceof pbi_ref_interface) || $handler->reference_exists($property)) {
            $new_instances = $handler->save($name, $property, $required_properties, $this->weight);
          }
        }
        else {
          watchdog('patternbuilder_importer', 'Meta type handler could not be loaded for "%property" of type "%type".', array(
            '%property' => $name,
            '%type' => $meta_type,
          ));
        }
      }
      else {
        watchdog('patternbuilder_importer', 'Meta type not found for "%property" of type "%type".', array(
          '%property' => $name,
          '%type' => $meta_type,
        ));
      }

      if ($new_instances) {
        $all_instances_created = array_merge($all_instances_created, $new_instances);
      }
    }

    return $all_instances_created;
  }

  /**
   * Get all meta type handler classes.
   *
   * @return array
   *   An array of handler classes keyed by meta type.
   */
  public static function meta_type_handlers() {
    $handlers = &drupal_static('pbi_factory.' . __FUNCTION__);
    if (!isset($handlers)) {
      $handlers = array(
        'field' => 'pbi_field',
        'field_ref' => 'pbi_field_ref',
        'field_group_ref' => 'pbi_field_group_ref',
        'field_group' => 'pbi_field_group',
        'field_collection' => 'pbi_field_collection',
        'paragraph' => 'pbi_field_paragraph',
        'field_link' => 'pbi_field_link',
        'field_file' => 'pbi_field_media',
        'field_image' => 'pbi_field_image',
        'field_audio' => 'pbi_field_audio',
        'field_video' => 'pbi_field_video',
      );
    }
    return $handlers;
  }

  /**
   * Retrieves the handler class for a given meta type.
   *
   * @param string $meta_type
   *   The meta type name.
   *
   * @return string|null
   *   The field handler class name.
   */
  public static function get_meta_type_handler($meta_type) {
    $handlers = static::meta_type_handlers();
    return isset($handlers[$meta_type]) ? $handlers[$meta_type] : NULL;
  }

  /**
   * Determine which type of object should be created based on property type.
   *
   * @param object $property
   *   Full property for determining the type.
   *
   * @return bool|string
   *   Object type or FALSE on not found.
   */
  public static function determine_meta_type(&$property) {
    $meta_type = FALSE;

    // Direct schema references.
    $meta_type_ref = static::resolve_reference_meta_type($property);
    if ($meta_type_ref) {
      return $meta_type_ref;
    }

    // Special field types.
    if (isset($property->entity)) {
      $entity_meta_type = 'field_' . pbi_utility::create_machine_name($property->entity);

      // Check dependencies of special fields to allow fallback to other
      // meta types.
      $handler_class = static::get_meta_type_handler($entity_meta_type);
      if ($handler_class && $handler_class::dependencies_met()) {
        $meta_type = $entity_meta_type;
      }
    }

    if (!$meta_type && isset($property->type)) {
      switch ($property->type) {
        case 'boolean':
        case 'integer':
        case 'number':
        case 'string':
        case 'text':
        case 'textarea':
          $meta_type = 'field';
          break;

        case 'object':
          $meta_type = 'field_group';
          // TODO: object with no property definition.
          // if (!isset($property->properties)) {
          // }
          break;

        case 'field_collection':
          // Special case forced in a field class.
          $meta_type = 'field_collection';
          break;

        case 'array':
          $meta_type = NULL;
          $items = pbi_utility::resolve_property_items($property);
          if (!empty($items->type)) {
            if ($items->type == 'object') {
              $meta_type = 'field_collection';
            }
            elseif ($items->type == '$ref') {
              $meta_type = 'paragraph';
            }
          }

          // Fallback to standard field.
          if (!isset($meta_type)) {
            $meta_type = 'field';
          }
          break;

        default:
          $meta_type = FALSE;
      }
    }

    // Reference detections for any metatype.
    if (isset($property->{'$ref'})) {
      if ($meta_type === 'field_group') {
        $meta_type = 'field_group_ref';
      }
      else {
        // Default to reference resolver.
        // Reference with no property type or base metatype defined.
        $meta_type = '$ref';
      }
    }

    return $meta_type;
  }

  /**
   * Determine which type of object should be created based on property type.
   *
   * @param object $property
   *   Full property for determining the type.
   *
   * @return bool|string
   *   Object type or FALSE on not found.
   */
  public static function resolve_reference_meta_type(&$property) {
    if (isset($property->{'$ref'}) && preg_match('@\.json$@', $property->{'$ref'})) {
      $ref_name = basename($property->{'$ref'}, '.json');
      if (patternbuilder_component_is_resolved($ref_name)) {
        // Store original schema before resolve.
        $override_prop = clone $property;
        // Exclude $ref from the override list since we no longer need it.
        unset($override_prop->{'$ref'});

        // Resolved schemas.
        patternbuilder_resolve_schema($property);

        // Redefine the values in the resolved $property with any values defined
        // in the overriding property (aka, the parent pattern or component.)
        // The \JsonSchema\RefResolver does not combine these during resolve.
        // The DrupalPatternBuilderRefResolver does keep these when loading from
        // cache.
        if (!empty($override_prop)) {
          // loop used in place of array_merge with object/array casting to
          // allow for individual property testing and cleaning.
          foreach ($override_prop as $key => $prop) {
            // Account for near empty properties and skip them.
            $prop_is_empty = FALSE;
            if (is_string($prop)) {
              $clean_prop = trim($prop);
              $prop_is_empty = empty($clean_prop);
            }
            if (!$prop_is_empty) {
              $property->{$key} = $prop;
            }
          }
        }

        // Single field schemas.
        if (patternbuilder_component_is_field($ref_name)) {
          $component = patternbuilder_components_load($ref_name);
          if (!empty($component->field_name)) {
            $property->_pb_base_field_name = $component->field_name;
          }
        }

        // Return nothing to allow normal type determination.
        return NULL;
      }
      else {
        // Paragraph bundle based schemas.
        return 'paragraph';
      }
    }
  }

}
