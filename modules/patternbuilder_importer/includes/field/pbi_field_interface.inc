<?php

/**
 * @file
 * Interface for creating Drupal fields for pattern properties.
 */

/**
 * Base Field interface.
 */
interface pbi_field_interface {

  /**
   * Determines if dependencies are met for this field handler.
   *
   * @return bool
   *   TRUE if the dependencies are met.
   */
  public static function dependencies_met();

  /**
   * Form for global per handler importer settings.
   *
   * @return array
   *   A form array keyed by the handler class name.
   */
  public static function settings_form();

  /**
   * Values for global per handler importer settings.
   *
   * @return array
   *   An array of handler settings.
   */
  public static function settings_values();

  /**
   * Returns a Drupal field type that is independent of any property definition.
   *
   * If this is defined then it should always be used as the field type.
   *
   * @return string
   *   A Drupal field type.
   */
  public static function global_field_type();

  /**
   * Returns a Drupal field value key.
   *
   * @return string
   *   A field value key.
   */
  public static function global_field_value_key();

  /**
   * Saves the values of the property.
   *
   * @param string $name
   *   The name of the property.
   * @param object $property
   *   The property object for a single property.
   * @param array $required_properties
   *   Array of required property names.
   * @param int $weight
   *   Field sorting weight.
   *
   * @return array
   *   An array of field instances created indexed numerically and not by
   *   field name to avoid collisions when merging.
   */
  public function save($name, $property, array $required_properties, &$weight);

  /**
   * Create a valid field name.
   *
   * @param string $name
   *   The name of the property.
   * @param object $property
   *   The property object for a single property.
   * @param int|string $id_override
   *   An ID override to lookup fields not part of this object.
   *
   * @return string
   *   A valid field name.
   */
  public function create_field_name($name, $property, $id_override = NULL);

}
