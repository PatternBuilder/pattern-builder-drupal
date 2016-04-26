<?php
/**
 * @file
 * Class for creating Drupal image fields for pattern properties.
 */

/**
 * Field image importer class.
 */
class pbi_field_image extends pbi_field_media {

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return parent::dependencies_met() && module_exists('image');
  }

  /**
   * {@inheritdoc}
   */
  protected static function field_property_map_suggestions() {
    $map = parent::field_property_map_suggestions();

    // Alt text set by file_entity_file_load().
    $map['alt'] = array('alt');

    // Title text set by file_entity_file_load().
    $map['title'] = array('title');

    return $map;
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return 'image';
  }

  /**
   * {@inheritdoc}
   */
  public static function global_allowed_file_types() {
    return array('image');
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_default_field_formatter($property, array $base, array $instance) {
    // @todo: set formatter image style?
    // Use the default image formatter for now.
    return array();
  }

}
