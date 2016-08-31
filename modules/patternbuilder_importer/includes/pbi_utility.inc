<?php

/**
 * @file
 * Utility class for import classes.
 */

/**
 * Importer utility class.
 */
class pbi_utility {

  /**
   * Case-insensitive string comparison with multibyte support.
   *
   * @param string $a
   *   The first string to check.
   * @param string $b
   *   The second string to check.
   *
   * @return bool
   *   TRUE if the value is set to "TRUE".
   */
  public static function strcasecmp($a, $b) {
    return drupal_strtolower($a) == drupal_strtolower($b);
  }

  /**
   * Check if a boolean value is set.
   *
   * @param mixed $value
   *   The value to check.
   *
   * @return bool
   *   TRUE if the value is set to "TRUE".
   */
  public static function is_boolean_set($value) {
    if (!isset($value)) {
      return FALSE;
    }

    if (is_string($value)) {
      return stripos($value, "true") !== FALSE;
    }

    return !empty($value);
  }

  /**
   * Resolves the property items.
   *
   * @param object $property
   *   The schema property.
   *
   * @return object
   *   An object with keys of:
   *   - "items" array|null: The resolved items array.
   *   - "validOf" string: Possible: '', 'oneOf', 'anyOf', 'allOf'.
   *   - "type" string: The type of the items.
   *   - "tuple" bool: TRUE if the items are considered a tuple.
   */
  public static function resolve_property_items($property) {
    $return = new stdClass();
    $return->validOf = NULL;
    $return->type = NULL;
    $return->tuple = FALSE;
    $return->items = NULL;

    if (!isset($property->items)) {
      return $return;
    }

    if (isset($property->items->{'$ref'})) {
      // Single reference.
      // @todo is this valid?
      // This was a single in hero.json for body_cotent, but there was
      // a dup $ref key for footer_content.
      $return->items = array($property->items);
      $return->type = '$ref';
    }
    elseif (is_array($property->items)) {
      // Items array.
      $return->items = $property->items;
    }
    else {
      // Items object.
      $items_key = NULL;
      foreach (array('oneOf', 'anyOf', 'allOf') as $valid_of_key) {
        if (isset($property->items->{$valid_of_key})) {
          $items_key = $valid_of_key;
          break;
        }
      }

      // Set items.
      if ($items_key) {
        // Validated items.
        if (is_array($property->items->{$items_key})) {
          $return->validOf = $items_key;
          $return->items = $property->items->{$items_key};
        }
      }
      else {
        // Single items definition.
        $return->items = array($property->items);
      }

      // Set type.
      if (isset($property->items->type)) {
        $return->type = $property->items->type;
      }
    }

    // Set type based on the items.
    if (!isset($return->type) && !empty($return->items)) {
      $item_type = NULL;
      $same_type_count = 0;
      $ref_count = 0;
      $items_count = count($return->items);
      foreach ($return->items as $item) {
        if (isset($item->type)) {
          if (!isset($item_type)) {
            $item_type = $item->type;
            $same_type_count++;
          }
          elseif ($item->type == $item_type) {
            $same_type_count++;
          }
        }
        elseif (isset($item->{'$ref'})) {
          $ref_count++;
        }
      }

      if ($same_type_count == $items_count) {
        // All items are the same type.
        $return->type = $item_type;
        $return->tuple = TRUE;
      }
      elseif ($ref_count == $items_count) {
        // All items are references.
        $return->type = '$ref';
      }
      else {
        // Mixed type.
        $return->type = 'object';
        $return->tuple = TRUE;
      }
    }

    return $return;
  }

  /**
   * Create a unique name with a maximum length.
   *
   * Desired pattern: [name space]_[id]_[name].
   * ID used to get around 32 field name character limit.
   * ID also keeps the field name unique per component to avoid collisions.
   *
   * @param string $ns_prefix
   *   The prefix for the name space, ie "field", "group".
   * @param string $name
   *   The name of the property.
   * @param int|string $id
   *   An ID override to lookup fields not part of this object.
   * @param int $max_length
   *   The maximum length of the name.
   *
   * @return string
   *   A unique name.
   */
  public static function create_unique_name($ns_prefix, $name, $id = NULL, $max_length = 32) {
    $max_length = $max_length > 0 ? $max_length : 32;

    // Get desired name.
    $desired = static::get_desired_name_parts($ns_prefix, $name, $id);

    // Set to the desired field name.
    $field_name = $desired['full_name'];

    // Adjust name if limit is exceeded.
    if (drupal_strlen($field_name) > $max_length) {
      // Create hash of desired name.
      $hash_name = md5($desired['full_name']);

      $short_hash_length = 8;
      $prefix_length = drupal_strlen($desired['prefix']);
      $ns_prefix_length = drupal_strlen($desired['ns']);
      $name_length = drupal_strlen($desired['name']);

      if (drupal_strlen($desired['id']) > $short_hash_length && $name_length <= ($max_length - ($ns_prefix_length + $short_hash_length + 2))) {
        // ID set to short hash.
        $field_name = $desired['ns'] . '_' . substr($hash_name, 0, $short_hash_length) . '_' . $desired['name'];
      }
      elseif (($prefix_length + 1) <= ($max_length - $short_hash_length)) {
        // Desired prefix + short hash for property name.
        $field_name = $desired['prefix'] . '_' . substr($hash_name, 0, $short_hash_length);
      }
      else {
        // Namespace + long hash.
        $field_name = $desired['ns'] . '_' . substr($hash_name, 0, $max_length - $ns_prefix_length - 1);
      }
    }

    return drupal_strtolower($field_name);
  }

  /**
   * Create the desired unique name.
   *
   * @param string $ns_prefix
   *   The prefix for the name space, ie "field", "group".
   * @param string $name
   *   The name of the property.
   * @param int|string|null $id
   *   An ID override to lookup fields not part of this object.
   *
   * @return string
   *   A unique name.
   */
  protected static function get_desired_name_parts($ns_prefix, $name, $id = NULL) {
    $parts = array('ns' => $ns_prefix);
    $prefix = $ns_prefix;

    // Id.
    if (!is_null($id)) {
      $parts['id'] = (string) $id;
      $prefix .= '_' . $parts['id'];
    }
    else {
      $parts['id'] = '';
    }

    // Set prefix.
    $parts['prefix'] = $prefix;

    // Ensure consistent machine name.
    $parts['name'] = static::create_machine_name($name);

    // Build full name.
    $parts['full_name'] = $parts['prefix'] . '_' . $parts['name'];

    return $parts;
  }

  /**
   * Creates a valid machine name.
   *
   * @param string $text
   *   The text to convert.
   *
   * @return string
   *   A machine name.
   */
  public static function create_machine_name($text) {
    return drupal_strtolower(preg_replace('@[^A-Za-z0-9\_]@', '_', $text));
  }

  /**
   * Check if a property is imported.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is imported.
   */
  public static function property_is_imported($property) {
    return !isset($property->options->import) || static::is_boolean_set($property->options->import);
  }

  /**
   * Determines if a property is a text area.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is a text area.
   */
  public static function property_is_textarea($property) {
    return isset($property->format) && (static::strcasecmp($property->format, 'textarea') || static::strcasecmp($property->format, 'html'));
  }

  /**
   * Determines if a property allows HTML markup.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property allows HTML.
   */
  public static function property_allows_html($property) {
    return isset($property->format) && static::strcasecmp($property->format, 'html');
  }

  /**
   * Determines if a property has set a collapsible option.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property has set a collapsible option.
   */
  public static function property_collapse_isset($property) {
    return isset($property->options->collapsed) || isset($property->options->disable_collapse);
  }

  /**
   * Determines if a property can be collapsed.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property can be collapsed.
   */
  public static function property_can_collapse($property) {
    $disabled = isset($property->options->disable_collapse) && static::is_boolean_set($property->options->disable_collapse);
    return !$disabled;
  }

  /**
   * Determines if a property is collapsed by default.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is collapsed by default.
   */
  public static function property_default_collapsed($property) {
    return isset($property->options->collapsed) && pbi_utility::is_boolean_set($property->options->collapsed);
  }

  /**
   * Process enum values to create allowed values.
   *
   * @param string $name
   *   The name of the property.
   * @param object $property
   *   The property object for a single property.
   * @param bool $is_required
   *   TRUE if the field is required.
   *
   * @return array
   *   An array of allowed values.
   */
  public static function property_allowed_values($name, $property, $is_required = FALSE) {
    // Determine enum values for the list allowed values.
    $enum_values = NULL;
    if (isset($property->enum)) {
      $enum_values = $property->enum;
    }
    elseif (isset($property->items)) {
      $resolved_items = static::resolve_property_items($property);

      // @todo: have utility function filter to valid values?
      // @todo: does flattening needs to handle more than just arrays of values?
      if (isset($resolved_items->validOf) && !empty($resolved_items->items)) {
        $valid_property_values = array();
        foreach ($resolved_items->items as $property_item) {
          if (isset($property_item->enum) && is_array($property_item->enum)) {
            if (static::strcasecmp($resolved_items->validOf, 'allOf')) {
              if ($valid_property_values) {
                $valid_property_values = array_intersect($valid_property_values, $property_item->enum);
              }
              else {
                $valid_property_values = $property_item->enum;
              }
            }
            else {
              // Build full array for anyOf and oneOf.
              $valid_property_values = array_merge($valid_property_values, $property_item->enum);
            }
          }
        }

        if ($valid_property_values) {
          if (static::strcasecmp($resolved_items->validOf, 'oneOf')) {
            $valid_property_values_counts = array_count_values($valid_property_values);
            $enum_values = array_keys(array_intersect($valid_property_values_counts, array(1)));
          }
          else {
            $enum_values = $valid_property_values;
          }
        }
      }
    }

    if (isset($enum_values)) {
      $enum_values = array_unique($enum_values);
    }

    $allowed_values = array();
    if (!empty($enum_values)) {
      $empty_label = t('- None -');
      foreach ($enum_values as $enum_value) {
        // Strip pipe since the field ui uses it to combine keys and values.
        $enum_key = strtr($enum_value, array('|' => '_'));

        if ($enum_value === "") {
          // Only set empty values if field is required.
          // Optional fields get the special _none option added by Drupal.
          if ($is_required) {
            $allowed_values[$enum_key] = $empty_label;
          }
        }
        else {
          $allowed_values[$enum_key] = $enum_value;
        }
      }
    }

    return $allowed_values;
  }

  /**
   * Determines the max number of items allowed for a property.
   *
   * @param object $property
   *   The property object.
   *
   * @return int|null
   *   The max number of items if defined.
   */
  public static function property_max_items($property) {
    if (isset($property->maxItems) && is_numeric($property->maxItems) && intval($property->maxItems) > 0) {
      // Schema defined value.
      return (int) $property->maxItems;
    }
    elseif (isset($property->type) && $property->type == 'array') {
      // Set max to unlimited.
      return FIELD_CARDINALITY_UNLIMITED;
    }
  }

  /**
   * Determines the min number of items allowed for a property.
   *
   * @param object $property
   *   The property object.
   *
   * @return int|null
   *   The min number of items if defined.
   */
  public static function property_min_items($property) {
    if (isset($property->minItems) && is_numeric($property->minItems) && intval($property->minItems) > 0) {
      return (int) $property->minItems;
    }
  }

  /**
   * Determines if a property is grid formatted.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is grid format.
   */
  public static function property_is_grid_format($property) {
    if (isset($property->format) && static::strcasecmp($property->format, 'grid')) {
      return TRUE;
    }
    return isset($property->items->format) && static::strcasecmp($property->items->format, 'grid');
  }

  /**
   * Determines the grid columns for a property.
   *
   * @param object $property
   *   The property object.
   *
   * @return int|null
   *   The grid columns defined.
   */
  public static function property_grid_columns($property) {
    if (isset($property->options->grid_columns) && is_numeric($property->options->grid_columns)) {
      $number = intval($property->options->grid_columns);
      if ($number > 0) {
        return $number;
      }
    }
  }

  /**
   * Determines if a property should be displayed in the preview.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is a preview field.
   */
  public static function property_is_preview($property) {
    return isset($property->options->preview) && static::is_boolean_set($property->options->preview);
  }

}
