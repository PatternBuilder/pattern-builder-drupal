<?php

/**
 * @file
 * Class for creating Drupal audio fields for pattern properties.
 */

/**
 * Field audio importer class.
 */
class pbi_field_audio extends pbi_field_media {

  /**
   * {@inheritdoc}
   */
  public static function global_allowed_file_types() {
    return array('audio');
  }

}
