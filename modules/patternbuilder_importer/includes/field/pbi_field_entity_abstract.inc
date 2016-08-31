<?php

/**
 * @file
 * Class for creating Drupal fields for schema entity object properties.
 */

/**
 * Abstract class for special entity object handlers.
 */
abstract class pbi_field_entity_abstract extends pbi_field {

  /**
   * Returns suggested schema properties for the Drupal field properties.
   *
   * @return array
   *   An array keyed by the Drupal field property name with values of an array
   *   of schema property names in order of preference.
   */
  public static function field_property_map_suggestions() {
    return array();
  }

  /**
   * Determines the map of Drupal field properties to schema properties.
   *
   * @param object $property
   *   The property object.
   *
   * @return array
   *   An array with keys of Drupal field property name and values of schema
   *   property name.
   */
  protected function field_property_map($property) {
    if (!isset($property->properties)) {
      return array();
    }

    $map = array();

    $suggestions = static::field_property_map_suggestions();
    if ($suggestions) {
      foreach ($suggestions as $field_prop => $schema_props) {
        foreach ($schema_props as $schema_prop) {
          if (is_array($schema_prop)) {
            // Property attribute match.
            if ($schema_prop) {
              foreach ($property->properties as $prop_name => $prop_value) {
                $attr_match = TRUE;
                foreach ($schema_prop as $attr_name => $attr_value) {
                  if (!isset($property->properties->$prop_name->{$attr_name}) || $property->properties->$prop_name->{$attr_name} != $attr_value) {
                    $attr_match = FALSE;
                    break;
                  }
                }

                if ($attr_match) {
                  $map[$field_prop] = $prop_name;
                  continue 3;
                }
              }
            }
          }
          elseif (isset($property->properties->{$schema_prop})) {
            // Property name match.
            $map[$field_prop] = $schema_prop;
            continue 2;
          }
        }
      }
    }

    return $map;
  }

  /**
   * {@inheritdoc}
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $map = $this->field_property_map($property);

    // Skip field if no properties could be mapped.
    if (empty($map)) {
      return FALSE;
    }

    // Set the map on the property so others can use it.
    $property->_pb_property_map = $map;

    // Process field.
    $field = parent::preprocess_field($name, $property, $required_properties, $weight);

    return $field;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_field_instance_settings($property, array $base, array $instance) {
    $settings = parent::determine_field_instance_settings($property, $base, $instance);

    // Add instance settings.
    if (!empty($property->_pb_property_map)) {
      $settings['patternbuilder']['property_map'] = patternbuilder_join_field_property_map($property->_pb_property_map);
    }

    return $settings;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_default_field_item($property, array $base, array $instance, array $allowed_values = array()) {
    if (empty($property->_pb_property_map)) {
      return parent::determine_default_field_item($property, $base, $instance, $allowed_values);
    }

    $item = $this->field_item_stub($property, $base);
    if (!empty($item)) {
      $map = $property->_pb_property_map;
      $required = isset($property->required) ? $property->required : array();

      // Process field schema properties.
      foreach (array_keys($item) as $field_prop) {
        if (isset($map[$field_prop]) && isset($property->properties->{$map[$field_prop]})) {
          $prop_is_required = in_array($map[$field_prop], $required);
          $prop_allowed_values = pbi_utility::property_allowed_values($map[$field_prop], $property->properties->{$map[$field_prop]}, $prop_is_required);
          $new_default_value = patternbuilder_importer_get_property_default_value($property->properties->{$map[$field_prop]}, $base, $instance, $prop_allowed_values);
          if (isset($new_default_value) && is_scalar($new_default_value)) {
            $item[$field_prop] = $new_default_value;
          }
        }
      }

      return array_filter($item) ? $item : NULL;
    }
  }

}
