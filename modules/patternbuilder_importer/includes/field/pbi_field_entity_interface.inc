<?php

/**
 * @file
 * Interface for creating Drupal fields for schema entity object properties.
 */

/**
 * Interface class for special entity object handlers.
 */
interface pbi_field_entity_interface extends pbi_field_interface {

  /**
   * Returns suggested schema properties for the Drupal field properties.
   *
   * @return array
   *   An array keyed by the Drupal field property name with values of an array
   *   of schema property names in order of preference.
   */
  public static function field_property_map_suggestions();

}
