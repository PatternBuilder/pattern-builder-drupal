<?php

/**
 * @file
 * Class for Drupal media file fields for schema entity object properties.
 */

/**
 * Class for media file entity object handlers.
 */
class pbi_field_media extends pbi_field_entity_abstract {

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return parent::dependencies_met() && module_exists('file') && module_exists('media');
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_defaults() {
    $defaults = array(
      'allowed_file_schemes' => array(),
      'browser_plugins' => array(),
    );
    $defaults += parent::settings_defaults();
    return $defaults;
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_form() {
    $form = parent::settings_form();
    $storage_id = static::settings_storage_id();
    $settings = static::settings_values();

    $visible_steam_wrappers = file_get_stream_wrappers(STREAM_WRAPPERS_VISIBLE);
    $scheme_options = array();
    foreach ($visible_steam_wrappers as $scheme => $scheme_info) {
      $scheme_options[$scheme] = check_plain($scheme_info['name']);
    }

    $classname = get_called_class();
    $form[$storage_id] = array(
      '#type' => 'fieldset',
      '#title' => $classname,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form[$storage_id]['allowed_file_schemes'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Allowed URI schemes'),
      '#options' => $scheme_options,
      '#default_value' => $settings['allowed_file_schemes'],
      '#description' => t('URI schemes which are allowed for this field. If no schemes are selected, they will all be available.'),
    );

    // Browser plugin.
    $browser_plugins = media_get_browser_plugin_info();
    $browser_plugin_options = array();
    foreach ($browser_plugins as $browser_plugin_name => $browser_plugin) {
      $browser_plugin_options[$browser_plugin_name] = check_plain($browser_plugin['title']);
    }

    $form[$storage_id]['browser_plugins'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Enabled browser plugins'),
      '#options' => $browser_plugin_options,
      '#default_value' => $settings['browser_plugins'],
      '#description' => t('Media browser plugins which are allowed for this field. If no plugins are selected, they will all be available.'),
    );

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return 'file';
  }

  /**
   * Global media file types.
   *
   * @return array
   *   An array of media file types independent of any property definitions.
   */
  public static function global_allowed_file_types() {
    return array();
  }

  /**
   * {@inheritdoc}
   */
  public static function field_property_map_suggestions() {
    $map = array();

    // Url created from file_create_url($uri).
    $map['url'] = array(
      'src',
      array('format' => 'url'),
      'url',
    );

    // Create property maps for the rendered markup.
    // Example: 'imageHTML', 'videoHTML'.
    $types = static::global_allowed_file_types();
    if ($types) {
      $rendered_props = array();
      foreach ($types as $type) {
        $rendered_props[] = pbi_utility::create_machine_name($type) . 'HTML';
      }

      $map[PATTERNBUILDER_PROPERTY_MAP_RENDERED_NAME] = $rendered_props;
    }

    return $map;
  }

  /**
   * Returns allowed file entity types.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of allowed file type machine names.
   */
  protected function media_allowed_file_types($property, array $base, array $instance) {
    // Set to global media types if defined.
    $types = static::global_allowed_file_types();
    if ($types) {
      return $types;
    }

    // Allow all by default.
    return array();
  }

  /**
   * Returns allowed file schemes.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of allowed file scheme machine names.
   */
  protected function media_allowed_file_schemes($property, array $base, array $instance) {
    $settings = static::settings_values();
    if (!empty($settings['allowed_file_schemes'])) {
      return array_filter($settings['allowed_file_schemes']);
    }

    return array(variable_get('file_default_scheme', 'public'));
  }

  /**
   * Returns allowed media module browser plugins.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of allowed media browser plugin machine names.
   */
  protected function media_allowed_browser_plugins($property, array $base, array $instance) {
    $settings = static::settings_values();
    if (!empty($settings['browser_plugins'])) {
      return array_filter($settings['browser_plugins']);
    }

    // Allow all by default.
    return array();
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_field_widget($property, array $base, array $instance) {
    // Get allowed settings.
    $allowed = array(
      'file_types' => $this->media_allowed_file_types($property, $base, $instance),
      'file_schemes' => $this->media_allowed_file_schemes($property, $base, $instance),
      'media_browsers' => $this->media_allowed_browser_plugins($property, $base, $instance),
    );

    // Create unique associative arrays.
    foreach ($allowed as $allowed_key => $allowed_values) {
      if (!empty($allowed[$allowed_key])) {
        $allowed_unique = array_unique(array_filter($allowed_values));
        $allowed[$allowed_key] = array_combine($allowed_unique, $allowed_unique);
      }
    }

    // Build widget settings.
    return array(
      'type' => 'media_generic',
      'settings' => array(
        'allowed_types' => $allowed['file_types'],
        'allowed_schemes' => $allowed['file_schemes'],
        'browser_plugins' => $allowed['media_browsers'],
      ),
    );
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_default_field_formatter($property, array $base, array $instance) {
    $formatter = $this->hook_default_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      $formatter = array(
        'type' => 'file_rendered',
        'settings' => array(
          'file_view_mode' => 'default',
        ),
      );
    }
    return $formatter;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_preview_field_formatter($property, array $base, array $instance) {
    $formatter = $this->hook_preview_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      $formatter = array(
        'type' => 'file_rendered',
        'label' => 'hidden',
        'settings' => array(
          'file_view_mode' => 'preview',
        ),
      );
    }
    return $formatter;
  }

}
