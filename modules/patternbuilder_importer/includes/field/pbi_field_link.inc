<?php

/**
 * @file
 * Class for creating Drupal link fields for pattern properties.
 */

/**
 * Field link importer class.
 */
class pbi_field_link extends pbi_field_entity_abstract {
  const LINK_TEXT_REQUIRED = 'required';
  const LINK_TEXT_OPTIONAL = 'optional';
  const LINK_TEXT_STATIC = 'value';
  const LINK_TEXT_NONE = 'none';

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return module_exists('link');
  }

  /**
   * {@inheritdoc}
   */
  public static function field_property_map_suggestions() {
    return array(
      'url' => array('href', 'url', array('format' => 'url')),
      'title' => array('text'),
      'attributes:title' => array('title'),
      'attributes:target' => array('target'),
      'attributes:rel' => array('rel'),
    );
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return 'link_field';
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_field_widget($property, array $base, array $instance) {
    return array('type' => 'link_field');
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_field_instance_settings($property, array $base, array $instance) {
    $settings = array();
    $parent_settings = parent::determine_field_instance_settings($property, $base, $instance);

    // Exit if no property map.
    if (empty($property->_pb_property_map)) {
      return $parent_settings;
    }

    // Get property values.
    $map = $property->_pb_property_map;
    $required = isset($property->required) ? $property->required : array();

    // Link url.
    if (isset($map['url'])) {
      // Required url = 0, Optional url = 1.
      $settings['url'] = in_array($map['url'], $required) ? 0 : 1;
    }
    else {
      // Optional url = 1.
      $settings['url'] = 1;
    }

    // Link text.
    if (isset($map['title'])) {
      $text_prop_name = $map['title'];
      $text_is_required = in_array($text_prop_name, $required);
      $text_allowed_values = pbi_utility::property_allowed_values($text_prop_name, $property->properties->{$text_prop_name}, $text_is_required);

      $settings['title'] = $text_is_required ? static::LINK_TEXT_REQUIRED : static::LINK_TEXT_OPTIONAL;

      if (count($text_allowed_values) == 1) {
        // Static title is used when only 1 allowed constant value.
        $settings['title'] = static::LINK_TEXT_STATIC;
        $settings['title_value'] = patternbuilder_importer_get_property_default_value($property->properties->{$text_prop_name}, $base, $instance, $text_allowed_values);
      }
    }
    else {
      $settings['title'] = static::LINK_TEXT_NONE;
    }

    // Link title attribute.
    if (isset($map['attributes:title'])) {
      $settings['attributes']['configurable_title'] = 1;
    }

    // Link rel attribute.
    if (isset($map['attributes:rel'])) {
      // Only default rel is supported.
      if (!empty($property->properties->{$map['attributes:rel']}->default)) {
        $settings['attributes']['rel'] = $property->properties->{$map['attributes:rel']}->default;
      }
    }

    // Link target attribute.
    if (isset($map['attributes:target'])) {
      $target_property = $property->properties->{$map['attributes:target']};
      $target_setting_default = LINK_TARGET_DEFAULT;

      if (!empty($target_property->default)) {
        $target_default_converted = $this->get_link_field_target_setting($target_property->default);
        if ($target_default_converted) {
          $target_setting_default = $target_default_converted;
        }
      }

      $allowed_target_settings = array();
      $allowed_target_settings_nonempty = array();
      if (!empty($target_property->enum)) {
        foreach ($target_property->enum as $target_enum) {
          $target_enum_setting = $this->get_link_field_target_setting($target_enum);
          if ($target_enum_setting) {
            $allowed_target_settings[$target_enum_setting] = $target_enum_setting;
          }
        }

        $allowed_target_settings_nonempty = array_diff($allowed_target_settings, array(LINK_TARGET_DEFAULT));
      }

      $target_setting = $target_setting_default;
      if (in_array($map['attributes:target'], $required)) {
        // Target is required, set to a non-empty default.
        if (!empty($target_setting_default) && $target_setting_default != LINK_TARGET_DEFAULT) {
          $target_setting = $target_setting_default;
        }
        elseif ($allowed_target_settings_nonempty) {
          $target_setting = reset($allowed_target_settings_nonempty);
        }
        else {
          $target_setting = LINK_TARGET_NEW_WINDOW;
        }
      }
      elseif (isset($allowed_target_settings[LINK_TARGET_DEFAULT]) && isset($allowed_target_settings[LINK_TARGET_NEW_WINDOW])) {
        // Provide option to user to "open in new window" or not.
        $target_setting = LINK_TARGET_USER;
      }
      elseif ($allowed_target_settings) {
        // Use first option in enums.
        $target_setting = reset($allowed_target_settings);
      }

      $settings['attributes']['target'] = $target_setting;
    }

    return $settings + $parent_settings;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_default_field_item($property, array $base, array $instance, array $allowed_values = array()) {
    // Parent handles direct mapped field schema properties.
    $parent_item = parent::determine_default_field_item($property, $base, $instance, $allowed_values);
    if (empty($property->_pb_property_map)) {
      return $parent_item;
    }

    $map = $property->_pb_property_map;
    $required = isset($property->required) ? $property->required : array();
    $item = is_array($parent_item) ? $parent_item : array();

    foreach ($map as $field_prop => $prop_name) {
      if (strpos($field_prop, 'attributes:') === 0 && isset($property->properties->{$prop_name})) {
        $prop_is_required = in_array($prop_name, $required);
        $prop_allowed_values = pbi_utility::property_allowed_values($prop_name, $property->properties->{$prop_name}, $prop_is_required);
        $new_default_value = patternbuilder_importer_get_property_default_value($property->properties->{$prop_name}, $base, $instance, $prop_allowed_values);
        if (isset($new_default_value) && is_scalar($new_default_value)) {
          $attr_key = preg_replace('@^attributes\:@', '', $field_prop);
          $item['attributes'][$attr_key] = $new_default_value;
        }
      }
    }

    return array_filter($item) ? $item : NULL;
  }

  /**
   * Returns the link field target setting for a give target attribute value.
   *
   * @param string $target
   *   The link target attribute value.
   *
   * @return string
   *   The link field setting value.
   */
  protected function get_link_field_target_setting($target) {
    if (empty($target)) {
      return LINK_TARGET_DEFAULT;
    }

    switch ($target) {
      case '_self':
        return LINK_TARGET_DEFAULT;

      case '_top':
        return LINK_TARGET_TOP;

      case '_blank':
        return LINK_TARGET_NEW_WINDOW;
    }
  }

}
