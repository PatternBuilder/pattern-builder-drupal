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
  protected static function field_property_map_suggestions() {
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
        $allowed[$allowed_key] = array_combine($allowed_values, $allowed_values);
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
    return array(
      'type' => 'file_rendered',
      'settings' => array(
        'file_view_mode' => 'default',
      ),
    );
  }

}
