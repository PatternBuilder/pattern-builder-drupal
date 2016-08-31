<?php

/**
 * @file
 * Class for referencing existing Drupal field groups.
 */

/**
 * Field group reference importer class.
 */
class pbi_field_group_ref extends pbi_field_group implements pbi_ref_interface {
  protected $referenceGroups = array();

  /**
   * {@inheritdoc}
   */
  public function save($name, $property, array $required_properties, &$weight) {
    $weight++;
    $processed = $this->preprocess_field($name, $property, $required_properties, $weight);
    if ($processed) {
      return array($processed);
    }

    return array();
  }

  /**
   * Pre-process property info into Drupal base and instance field arrays.
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
   * @return array|bool
   *   Combined array of group and children instances, false on failure.
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $ref_group = $this->get_reference_group($property);
    if (!isset($ref_group->group_name)) {
      // Fail-safe: The existence of the reference should be checked first.
      return FALSE;
    }

    // Set group name as ref group so other methods can use it.
    $this->groupName = $ref_group->group_name;

    // Clone the group with fields.
    return $this->clone_field_group($ref_group, array(
      'property_name' => $name,
      'property' => $property,
      'weight' => $weight,
      'group_name' => $this->groupName,
      'entity_type' => $this->entityType,
      'bundle' => $this->bundleName,
    ));
  }

  /**
   * {@inheritdoc}
   */
  public function reference_exists($property) {
    $group = $this->get_reference_group($property);
    return isset($group->group_name);
  }

  /**
   * Returns the field name from the property reference.
   *
   * @param object $property
   *   The property object.
   *
   * @return object|null
   *   The reference loaded group object.
   */
  protected function get_reference_group($property) {
    if (isset($property->{'$ref'})) {
      $property_ref = $property->{'$ref'};

      if (!array_key_exists($property_ref, $this->referenceGroups)) {
        $group_name = $this->get_reference_group_name($property);
        if ($group_name) {
          $group = static::field_group_load_single($group_name);
          if ($group) {
            $this->referenceGroups[$property_ref] = $group;
          }
        }
      }

      return $this->referenceGroups[$property_ref];
    }
  }

  /**
   * Returns the group name from the property reference.
   *
   * @param object $property
   *   The property object.
   *
   * @return string|null
   *   The reference group name.
   */
  protected function get_reference_group_name($property) {
    if (isset($property->{'$ref'})) {
      $machine_name = basename(preg_replace('/#.*$/', '', $property->{'$ref'}), '.json');
      $lookup = patternbuilder_components_load($machine_name);
      if (isset($lookup->id)) {
        $ref_property_name = basename($property->{'$ref'});
        return pbi_utility::create_unique_name(static::NAME_PREFIX, $ref_property_name, $lookup->id, static::NAME_MAX_LEN);
      }
    }
  }

}
