<?php

/**
 * @file
 * Class for creating Drupal field collections for pattern properties.
 */

/**
 * Field collection importer class.
 */
class pbi_field_collection extends pbi_field implements pbi_collapsible_interface {
  private $childEntityType = 'field_collection_item';

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return module_exists('field_collection');
  }

  /**
   * {@inheritdoc}
   */
  public function is_collapsible($property) {
    return module_exists('field_collection_fieldset') && pbi_utility::property_can_collapse($property);
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
    if ($field = $this->preprocess_field($name, $property, $required_properties, $weight)) {
      $field_instance = $this->add_to_bundle($field);
      if ($field_instance) {
        $this->process_child_fields($name, $property, $field);
        return array($field_instance);
      }
      return array();
    }
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return 'field_collection';
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_field_widget($property, array $base, array $instance) {
    $widget = array();
    if ($this->is_collapsible($property)) {
      $widget = array(
        'type' => 'field_collection_fieldset',
        'settings' => array(
          'field_as_label' => '',
          'replace_label' => 1,
          'collapsed' => (int) $this->collapsible_default_collapsed($property),
          'wrapper_collapsible' => 1,
          'last_item_collapsed' => 1,
          'new_item_open' => 1,
        ),
      );
    }
    else {
      $widget = array(
        'type' => 'field_collection_embed',
      );
    }

    return $widget;
  }

  /**
   * {@inheritdoc}
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $field = parent::preprocess_field($name, $property, $required_properties, $weight);

    if (!empty($field['instance']) && isset($property->items)) {
      $resolved_items = pbi_utility::resolve_property_items($property);

      // Set title.
      if (!empty($property->items->title)) {
        $field['instance']['label'] = $property->items->title;
      }

      // Set description.
      if (!empty($property->items->description)) {
        $field['instance']['description'] = $property->items->description;
      }

      // Default cardinality to unlimited if not defined, instead of Drupal
      // default of 1.
      if (!isset($field['base']['cardinality'])) {
        $field['base']['cardinality'] = FIELD_CARDINALITY_UNLIMITED;
      }
      elseif ($field['base']['cardinality'] > 0 && $resolved_items->tuple && $resolved_items->items) {
        // Tuple cardinality.
        // @todo: is this the expected behavior for tuples?
        $field['base']['cardinality'] = max(1, floor($field['base']['cardinality'] / count($resolved_items->items)));
      }
    }

    return $field;
  }

  /**
   * Process child fields.
   *
   * @param string $name
   *   The name of the parent property.
   * @param object $property
   *   The parent property.
   * @param array $field
   *   The field array of the parent base/instance.
   */
  protected function process_child_fields($name, $property, array $field) {
    // Exit if no field name.
    if (empty($field['base']['field_name'])) {
      return;
    }

    // Ensure field collection exists.
    $instance = field_info_instance($this->entityType, $field['base']['field_name'], $this->bundleName);
    if (empty($instance)) {
      // TODO: throw pbi field exception?
      watchdog('patternbuilder_importer', 'Child fields could not be created for field collection %field', array('%field' => $field['base']['field_name']), WATCHDOG_WARNING);
    }

    // Determine properties to use for the children.
    $props_object = NULL;
    if (isset($property->properties)) {
      $props_object = $property->properties;
    }
    else {
      $items = pbi_utility::resolve_property_items($property);
      if (!empty($items->items)) {
        // Normal field collection processing.
        if (count($items->items) == 1) {
          $top_item = reset($items->items);
          if (isset($top_item->properties)) {
            $props_object = $top_item->properties;
          }
        }
      }

      // Build pseudo properties for arrays of mixed objects / tuples.
      if (!isset($props_object) && !empty($items->tuple)) {
        $props = array();
        foreach ($items->items as $item_index => $item) {
          $prop_name = PATTERNBUILDER_PROPERTY_PREFIX_TUPLE_ITEM . '::' . patternbuilder_get_property_real_name($name) . '::' . $item_index;

          // Force type as field_collection.
          // @see pb_factory::determine_meta_type()
          $item->type = 'field_collection';

          $props[$prop_name] = $item;
        }

        $props_object = (object) $props;
      }
    }

    // Exit if children cannot be found.
    if (empty($props_object)) {
      return;
    }

    // Process child fields.
    $child_base_id = $this->id . '_' . $name;
    $child_id = $child_base_id;
    $child_weight = 0;
    $child_bundle_name = $field['base']['field_name'];
    $child_required_fields = isset($property->items->required) ? $property->items->required : array();

    $child_factory = patternbuilder_importer_create_factory($child_id, $child_bundle_name, $child_weight, $this->childEntityType);
    if (isset($child_factory)) {
      $child_factory->process_properties($props_object, $child_required_fields);
    }
  }

}
