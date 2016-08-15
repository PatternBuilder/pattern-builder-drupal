<?php

/**
 * @file
 * Class for referencing existing Drupal fields.
 */

/**
 * Reference field importer class.
 */
class pbi_field_ref extends pbi_field implements pbi_ref_interface {
  protected $referenceInstances = array();

  /**
   * {@inheritdoc}
   */
  public function __construct($id, $bundle_name, $entity_type = NULL) {
    parent::__construct($id, $bundle_name, $entity_type);
    $this->createFieldBase = FALSE;
  }

  /**
   * {@inheritdoc}
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $instance = $this->find_reference_field_instance($property);
    if (empty($instance['field_name'])) {
      // Fail-safe: The existence of the reference should be checked first.
      return FALSE;
    }

    $base = field_info_field($instance['field_name']);

    // Set required to the same as the reference.
    $is_required = isset($instance['required']) && $instance['required'] ? 1 : 0;

    // Determine if field is required.
    if (!empty($required_properties) && in_array($name, $required_properties)) {
      $is_required = 1;
    }

    // Set allowed values.
    $allowed_values = pbi_utility::property_allowed_values($name, $property, $is_required);

    $instance['entity_type'] = $this->entityType;
    $instance['bundle'] = $this->bundleName;
    if (isset($property->title)) {
      $instance['label'] = $property->title;
    }
    if (isset($property->description)) {
      $instance['description'] = $property->description;
    }
    $instance['widget']['weight'] = $weight;
    $instance['required'] = $is_required;
    $instance['settings']['patternbuilder']['property_name'] = $name;
    $instance['settings']['patternbuilder']['parent_property_names'] = '';

    if (isset($property->readonly)) {
      $instance['settings']['patternbuilder']['readonly'] = pbi_utility::is_boolean_set($property->readonly);
    }
    if (isset($property->options->hidden)) {
      $instance['settings']['patternbuilder']['hidden'] = pbi_utility::is_boolean_set($property->options->hidden);
    }

    // Set the default value.
    $default_item = $this->determine_default_field_item($property, $base, $instance, $allowed_values);
    if (isset($default_item)) {
      $instance['default_value'][0] = $default_item;
    }

    // Build combined return.
    $return = array(
      'base' => $base,
      'instance' => $instance,
    );

    // Allow others to alter.
    $context = array(
      'property' => $property,
      'handler' => $this,
    );
    drupal_alter('patternbuilder_importer_field_import', $return, $context);

    return $return;
  }

  /**
   * {@inheritdoc}
   */
  public function reference_exists($property) {
    $instance = $this->get_reference_field_instance($property);
    return !empty($instance['field_name']);
  }

  /**
   * Returns the field name from the property reference.
   *
   * @param object $property
   *   The property object.
   *
   * @return string
   *   The reference field name.
   */
  protected function get_reference_field_instance($property) {
    if ($property->{'$ref'}) {
      if (!array_key_exists($property->{'$ref'}, $this->referenceInstances)) {
        // Create the name.
        $this->referenceInstances[$property->{'$ref'}] = static::find_reference_field_instance($property);
      }

      return $this->referenceInstances[$property->{'$ref'}];
    }
  }

  /**
   * Finds the field instance corresponding to the property reference.
   *
   * Note: Only references to paragraphs_items currently is supported.
   *
   * @param object $property
   *   The property object.
   *
   * @return array|null
   *   The extracted Drupal field instance.
   */
  public static function find_reference_field_instance($property) {
    // Reference in the form of "quote.json#/properties/quotation".
    if (isset($property->{'$ref'})) {
      $schema_parts = explode('#/', $property->{'$ref'});
      if (count($schema_parts) > 1) {
        $machine_name = array_shift($schema_parts);
        $machine_name = preg_replace('@\.json$@', '', $machine_name);
        $ref_tail = array_pop($schema_parts);
        $ref_path_parts = explode('/', $ref_tail);
        $ref_property_name = array_pop($ref_path_parts);

        if ($machine_name && $ref_property_name) {
          $component = patternbuilder_components_load($machine_name);
          if (isset($component->id)) {
            // Determine if reference is allowed based on the status.
            if (!empty($component->status)) {
              // Log error for references to non-imported schemas.
              if (!patternbuilder_component_is_imported($component)) {
                watchdog('patternbuilder_importer', 'The property references a schema that is not imported due to the status "@status": "@ref"', array(
                  '@ref' => $property->{'$ref'},
                  '@status' => patternbuilder_pattern_status_get_label($component->status),
                ), WATCHDOG_ERROR);

                // Skip reference.
                return array();
              }

              // Log warning for references to non-creatable schemas.
              if (!patternbuilder_component_is_creatable($component)) {
                watchdog('patternbuilder_importer', 'The property references a schema that is not creatable due to the status "@status": "@ref"', array(
                  '@ref' => $property->{'$ref'},
                  '@status' => patternbuilder_pattern_status_get_label($component->status),
                ), WATCHDOG_WARNING);

                // Skip reference.
                return array();
              }
            }

            // Find bundle field matching the property reference.
            $field_instance = patternbuilder_field_info_property_instance($component->machine_name, array($ref_property_name));
            if ($field_instance) {
              return $field_instance;
            }
          }
        }
      }
    }
  }

}
