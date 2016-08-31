<?php

/**
 * @file
 * Class for creating Drupal field groups for pattern properties.
 */

/**
 * Field group importer class.
 */
class pbi_field_group implements pbi_field_interface, pbi_collapsible_interface {
  const NAME_PREFIX = 'group';
  const NAME_MAX_LEN = 32;

  protected $id;
  protected $bundleName;
  protected $entityType = 'paragraphs_item';
  protected $groupName;
  protected $group;

  /**
   * Class Construct.
   *
   * @param int|string $id
   *   ID for the pattern.
   * @param string $bundle_name
   *   The machine name of the bundle.
   * @param string|null $entity_type
   *   The entity type. If not provided, then the default entity type is used.
   */
  public function __construct($id, $bundle_name, $entity_type = NULL) {
    $this->id = $id;
    $this->bundleName = $bundle_name;
    if (isset($entity_type)) {
      $this->entityType = $entity_type;
    }
  }

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return module_exists('field_group');
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_form() {
    return array();
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_values() {
    return array();
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return NULL;
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_value_key() {
    return NULL;
  }

  /**
   * {@inheritdoc}
   */
  public function is_collapsible($property) {
    return pbi_utility::property_can_collapse($property);
  }

  /**
   * {@inheritdoc}
   */
  public function collapsible_default_collapsed($property) {
    return pbi_utility::property_default_collapsed($property);
  }

  /**
   * {@inheritdoc}
   */
  public function save($name, $property, array $required_properties, &$weight) {
    $weight++;
    $this->groupName = $this->create_group_name($name, $property);
    $this->create_field_group($name, $property, $weight);
    $new_id = $this->id . '_' . $name;

    if (isset($property->properties) && isset($this->group)) {
      $processed = array();
      $processed['group'] = $this->group;
      $factory = patternbuilder_importer_create_factory($new_id, $this->bundleName, $weight, $this->entityType);
      if (isset($factory)) {
        $required_properties = isset($property->required) ? $property->required : array();
        $field_instances = $factory->process_properties($property->properties, $required_properties);
        if ($field_instances) {
          $this->add_fields_to_group($this->group, $field_instances, $name);
          $processed['children'] = $field_instances;
        }
        else {
          field_group_group_export_delete($this->group);
        }
      }

      return array($processed);
    }

    return array();
  }

  /**
   * Creates field groups for use in property objects.
   *
   * @param string $property_name
   *   The property name.
   * @param object $property
   *   The property object.
   * @param int $weight
   *   Ordering Weight.
   */
  protected function create_field_group($property_name, $property, $weight) {
    $group = $this->create_field_group_stub(array(
      'property_name' => $property_name,
      'property' => $property,
      'weight' => $weight,
      'group_name' => $this->groupName,
      'entity_type' => $this->entityType,
      'bundle' => $this->bundleName,
    ));

    $existing_group = static::field_group_load_single($this->groupName, $this->entityType, $this->bundleName);
    if (!empty($existing_group->group_name)) {
      // Use existing id.
      $group->id = $existing_group->id;
      $group->export_type = $existing_group->export_type;
    }
    else {
      // Ensure these are not set to force an insert.
      // See field_group_group_save().
      unset($group->id, $group->export_type);
    }

    // Save field group.
    field_group_group_save($group);

    // Set created group.
    $this->group = static::field_group_load_single($this->groupName, $this->entityType, $this->bundleName);
  }

  /**
   * Updates the created group with the associated field names.
   *
   * @param object $group
   *   The field group object to attach the fields.
   * @param array $field_instances
   *   An array of field instances to add to the group.
   * @param string $group_property_name
   *   The property name of the group.
   * @param array $group_parent_property_names
   *   An array of parent property names in hierarchical order.
   */
  private function add_fields_to_group($group, array $field_instances, $group_property_name = NULL, array $group_parent_property_names = array()) {
    if (empty($field_instances) || empty($group)) {
      return;
    }

    // Resolve name from group if not provided.
    if (empty($group_property_name)) {
      $group_property_name = $this->get_field_group_property_name($group);
    }

    // Empty out the children so the array doesn't fill up on every run with
    // historical names. If child fields exist but aren't part of the group
    // the field will not be destroyed.
    $group->children = array();

    // Ensure parent properties.
    $group_parent_property_names = !empty($group_parent_property_names) ? $group_parent_property_names : array();

    // Build parent properties for child groups.
    $child_group_parent_property_names = $group_parent_property_names;
    if ($group_property_name) {
      $child_group_parent_property_names[] = $group_property_name;
    }

    $processed = array();
    foreach ($field_instances as $field_instance) {
      if (isset($field_instance['field_name'])) {
        // Fields.
        $field_name = $field_instance['field_name'];
        if (!isset($processed[$field_name])) {
          // Add field to group.
          $group->children[] = $field_name;

          // Store group property name in the instance settings.
          if ($this->set_child_field_instance_parents($field_instance, $group_property_name, $group_parent_property_names)) {
            field_update_instance($field_instance);
          }
        }
      }
      elseif (isset($field_instance['group']->group_name)) {
        // Groups.
        // Process child fields to set proper parent properties.
        if (!empty($field_instance['children'])) {
          $child_group_property_name = $this->get_field_group_property_name($field_instance['group']);
          $this->add_fields_to_group($field_instance['group'], $field_instance['children'], $child_group_property_name, $child_group_parent_property_names);
        }

        // Add child group to the group.
        $group->children[] = $field_instance['group']->group_name;
      }
    }

    // Update the group.
    field_group_group_save($group);
  }

  /**
   * Updates the field instance parent names.
   *
   * @param array $field_instance
   *   The child field instance array.
   * @param string $group_property_name
   *   The group property name.
   * @param array $group_parent_property_names
   *   The child's parent property names in hierarchical order.
   *
   * @return bool
   *   TRUE if the instance array was updated.
   */
  protected function set_child_field_instance_parents(array &$field_instance, $group_property_name, array $group_parent_property_names) {
    if (!isset($group_property_name)) {
      return FALSE;
    }

    if (!empty($group_parent_property_names)) {
      $parent_property_name_chain = implode(PATTERNBUILDER_PROPERTY_DELIMITER, $group_parent_property_names);
      $parent_property_name_chain .= PATTERNBUILDER_PROPERTY_DELIMITER . $group_property_name;
    }
    else {
      $parent_property_name_chain = $group_property_name;
    }

    if (!empty($field_instance['settings']['patternbuilder']['parent_property_names'])) {
      $existing_parent_name_chain = $field_instance['settings']['patternbuilder']['parent_property_names'];
      // Only update instance for new group children.
      if (strpos($existing_parent_name_chain, $parent_property_name_chain) === FALSE) {
        $field_instance['settings']['patternbuilder']['parent_property_names'] = $parent_property_name_chain . PATTERNBUILDER_PROPERTY_DELIMITER . $existing_parent_name_chain;
        return TRUE;
      }
    }
    else {
      $field_instance['settings']['patternbuilder']['parent_property_names'] = $parent_property_name_chain;
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Returns the groups property name if set.
   *
   * @param object $group
   *   The group object.
   *
   * @return string|null
   *   The property name.
   */
  protected function get_field_group_property_name($group) {
    if (!empty($group->children)) {
      foreach ($group->children as $c => $child_name) {
        if (stripos($child_name, 'group_') === FALSE) {
          $field_instance = field_info_instance($group->entity_type, $child_name, $group->bundle);
          if ($field_instance) {
            $pb_settings = _patternbuilder_field_instance_settings($field_instance);
            if (!empty($pb_settings['parent_property_names_array'])) {
              // Last item is this group's property name.
              return array_pop($pb_settings['parent_property_names_array']);
            }
          }
        }
      }
    }

    return NULL;
  }

  /**
   * Creates a stub field group object without saving.
   *
   * @param array $values
   *   An array with the following values for the new group:
   *   - group_name: The field group machine name.
   *   - entity_type: The entity type.
   *   - bundle_name: The entity bundle name.
   *   - property_name: (optional) The schema property name.
   *   - property: (optional) The schema property object.
   *   - weight: (optional) The field weight.
   *
   * @return object
   *   The stub group object, not saved.
   */
  protected function create_field_group_stub(array $values) {
    $group_format_type = 'fieldset';
    $group_formatter = 'open';
    $group_title = '';
    $group_description = '';
    $group_classes = array();

    // Set a property name.
    $property_name = isset($values['property_name']) ? $values['property_name'] : '';
    $group_title = $property_name ? $property_name : $values['group_name'];

    // Process property object.
    $property = NULL;
    if (isset($values['property'])) {
      $property = $values['property'];

      if (isset($property->title)) {
        $group_title = $property->title;
      }

      if (isset($property->description)) {
        $group_description = $property->description;
      }

      // Collapsible.
      if ($this->is_collapsible($property)) {
        $group_formatter = $this->collapsible_default_collapsed($property) ? 'collapsed' : 'collapsible';
      }

      // Grid layout.
      $grid_settings = patternbuilder_grid_layout_global_settings();
      if (!empty($grid_settings['row_class']) && pbi_utility::property_is_grid_format($property)) {
        $group_classes[] = $grid_settings['row_class'];
      }

      // Grid columns of child in a grid.
      if (!empty($grid_settings['col_class_prefix']) && ($grid_columns = pbi_utility::property_grid_columns($property))) {
        $group_classes[] = $grid_settings['col_class_prefix'] . intval(trim($grid_columns));
      }
    }

    // Group stub object.
    $group = (object) array(
      'identifier' => $values['group_name'] . '|' . $values['entity_type'] . '|' . $values['bundle'] . '|form',
      'group_name' => $values['group_name'],
      'entity_type' => $values['entity_type'],
      'bundle' => $values['bundle'],
      'mode' => 'form',
      'label' => $group_title,
      'children' => array(),
      'weight' => isset($values['weight']) ? $values['weight'] : 0,
      'format_type' => $group_format_type,
      'format_settings' => array(
        'formatter' => $group_formatter,
        'instance_settings' => array(
          'description' => $group_description,
          'classes' => $group_classes ? implode(' ', $group_classes) : '',
          'required_fields' => 1,
        ),
      ),
    );

    return $group;
  }

  /**
   * Clone a group with children.
   *
   * @param object $base_group
   *   The field group object to clone.
   * @param array $values
   *   An array with the following values for the new group:
   *   - entity_type: The entity type.
   *   - bundle_name: The entity bundle name.
   *   - property_name: (optional) The schema property name.
   *   - property: (optional) The schema property object.
   *   - weight: (optional) The field weight.
   *   - parent_property_names: (optional) An array of parent property names
   *     in hierarchical order.
   *
   * @return array
   *   Combined array of group and children instances.
   */
  protected function clone_field_group($base_group, array $values) {
    if (empty($base_group->group_name)) {
      return array();
    }

    // Check if there is an existing group.
    $group = static::field_group_load_single($base_group->group_name, $values['entity_type'], $values['bundle']);
    if (empty($group->group_name)) {
      // New group - clone referenced group.
      $group = clone($base_group);
      unset($group->id, $group->export_type);
    }

    // Create stub for overrides.
    $values['group_name'] = $base_group->group_name;
    $new_group = $this->create_field_group_stub($values);

    // Set common group values needed for processing.
    $new_group_property_name = isset($values['property_name']) ? $values['property_name'] : $this->get_field_group_property_name($base_group);
    $new_group_parent_property_names = !empty($values['parent_property_names']) ? $values['parent_property_names'] : array();

    // Build parent properties for child groups.
    $child_group_parent_property_names = $new_group_parent_property_names;
    if ($new_group_property_name) {
      $child_group_parent_property_names[] = $new_group_property_name;
    }

    // Override properties per instance.
    $group->label = $new_group->label;
    $group->entity_type = $new_group->entity_type;
    $group->bundle = $new_group->bundle;
    $group->weight = $new_group->weight;
    $group->identifier = $new_group->identifier;

    // Ensure children match the base group.
    $group->children = $base_group->children;

    // Format settings overrides.
    if (!isset($group->format_settings)) {
      $group->format_settings = $new_group->format_settings;
    }
    else {
      // Set formatter only if the reference defines a collapsed option.
      if (isset($values['property']) && pbi_utility::property_collapse_isset($values['property'])) {
        $group->format_settings['formatter'] = $new_group->format_settings['formatter'];
      }

      // Set description if this reference defines one.
      if (!empty($new_group->format_settings['instance_settings']['description'])) {
        $group->format_settings['instance_settings']['description'] = $new_group->format_settings['instance_settings']['description'];
      }
    }

    // Save the new / updated group.
    field_group_group_save($group);

    // Add children field instances for this bundle.
    $child_instances = array();
    if (!empty($group->children)) {
      foreach ($group->children as $c => $child_name) {
        if (stripos($child_name, 'group_') === 0 && ($base_child_group = static::field_group_load_single($child_name, $base_group->entity_type, $base_group->bundle))) {
          // Group.
          $child_group_values = array(
            'group_name' => $child_name,
            'entity_type' => $group->entity_type,
            'bundle' => $group->bundle,
            'weight' => $group->weight + $c + 1,
            'property_name' => $this->get_field_group_property_name($base_child_group),
            'parent_property_names' => $child_group_parent_property_names,
          );

          $new_child_instance = $this->clone_field_group($base_child_group, $child_group_values);
          $child_instances[] = $new_child_instance;
        }
        elseif (($base_child_instance = field_info_instance($base_group->entity_type, $child_name, $base_group->bundle)) && isset($base_child_instance['id'])) {
          // Field.
          $new_child_instance = array(
            'id' => NULL,
            'entity_type' => $group->entity_type,
            'bundle' => $group->bundle,
          );
          $new_child_instance += $base_child_instance;

          // Store group property name in the instance settings.
          $this->set_child_field_instance_parents($new_child_instance, $new_group_property_name, $new_group_parent_property_names);

          $existing_new_child_instance = field_info_instance($new_child_instance['entity_type'], $new_child_instance['field_name'], $new_child_instance['bundle']);
          if (empty($existing_new_child_instance)) {
            // Create a new field instance.
            $new_child_instance = field_create_instance($new_child_instance);
          }
          else {
            // Update the existing instance with new settings.
            $existing_new_child_instance['settings']['patternbuilder'] = $new_child_instance['settings']['patternbuilder'];
            field_update_instance($existing_new_child_instance);
            $new_child_instance = $existing_new_child_instance;
          }

          $child_instances[] = $new_child_instance;
        }
      }
    }

    return array(
      'group' => $group,
      'children' => $child_instances,
    );
  }

  /**
   * Create a valid Drupal group name.
   *
   * @param string $name
   *   The name of the property.
   *
   * @return string
   *   A valid Drupal group name.
   */
  protected function create_group_name($name, $property) {
    return $this->create_field_name($name, $property);
  }

  /**
   * {@inheritdoc}
   */
  public function create_field_name($name, $property, $id_override = NULL) {
    $id = is_null($id_override) ? (string) $this->id : (string) $id_override;
    return pbi_utility::create_unique_name(static::NAME_PREFIX, $name, $id, static::NAME_MAX_LEN);
  }

  /**
   * Loads a single field group for the given conditions.
   *
   * @param string $group_name
   *   The name of the group.
   * @param string $entity_type
   *   The name of the entity.
   * @param string $bundle_name
   *   The name of the bundle.
   * @param string $mode
   *   The view mode to load. Defaults to "form" if not provided.
   *
   * @return object|null
   *   A single loaded group object for the first found group.
   */
  public static function field_group_load_single($group_name = NULL, $entity_type = NULL, $bundle_name = NULL, $mode = NULL) {
    $groups = static::field_group_load($group_name, $entity_type, $bundle_name, $mode);
    return $groups ? reset($groups) : NULL;
  }

  /**
   * Loads all field groups for the given conditions.
   *
   * Based on field_group_load_field_group() with addition of optional
   * parameters.
   *
   * @param string $group_name
   *   The name of the group.
   * @param string $entity_type
   *   The name of the entity.
   * @param string $bundle_name
   *   The name of the bundle.
   * @param string $mode
   *   The view mode to load. Defaults to "form" if not provided.
   *
   * @return array|null
   *   An array of loaded group objects.
   */
  public static function field_group_load($group_name = NULL, $entity_type = NULL, $bundle_name = NULL, $mode = NULL) {
    $conditions = array();
    if (isset($group_name)) {
      $conditions['group_name'] = $group_name;
    }

    if (isset($entity_type)) {
      $conditions['entity_type'] = $entity_type;
    }

    if (isset($bundle_name)) {
      $conditions['bundle'] = $bundle_name;
    }

    if (isset($mode)) {
      $conditions['mode'] = $mode;
    }

    if (empty($conditions)) {
      return NULL;
    }

    // Default to form.
    if (!isset($conditions['mode'])) {
      $conditions['mode'] = 'form';
    }

    $identifier = NULL;
    if (count($conditions) == 4) {
      $identifier = implode('|', $conditions);
    }

    ctools_include('export');
    $groups = array();
    if ($identifier) {
      $objects = ctools_export_load_object('field_group', 'conditions', array(
        'identifier' => $identifier,
      ));
    }
    else {
      $objects = ctools_export_load_object('field_group', 'conditions', $conditions);
    }

    if (!empty($objects)) {
      foreach ($objects as $id => $object) {
        if ($object && isset($object->data)) {
          $groups[$id] = field_group_unpack($object);
        }
      }
    }

    return $groups;
  }

}
