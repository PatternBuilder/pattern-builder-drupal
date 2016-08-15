<?php

/**
 * @file
 * Class for creating Drupal video fields for pattern properties.
 */

/**
 * Field video importer class.
 */
class pbi_field_video extends pbi_field_media {

  /**
   * {@inheritdoc}
   */
  public static function global_allowed_file_types() {
    return array('video');
  }

  /**
   * {@inheritdoc}
   */
  public static function field_property_map_suggestions() {
    $map = parent::field_property_map_suggestions();
    $rendered_key = PATTERNBUILDER_PROPERTY_MAP_RENDERED_NAME;

    // Legacy support for 'video_src'.
    if (isset($map[$rendered_key])) {
      array_unshift($map[$rendered_key], 'video_src');
    }
    else {
      $map[$rendered_key] = array('video_src');
    }

    return $map;
  }

}
