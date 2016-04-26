<?php
/**
 * @file
 * Class for creating Drupal paragraph fields for pattern properties.
 */

/**
 * Paragraph field importer class.
 */
class pbi_field_paragraph extends pbi_field {

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return module_exists('paragraphs');
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return 'paragraphs';
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_field_widget($property, array $base, array $instance) {
    return array('type' => 'patternbuilder_paragraphs_embed');
  }

  /**
   * {@inheritdoc}
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $field = parent::preprocess_field($name, $property, $required_properties, $weight);

    // Default settings.
    $field['instance']['settings']['title'] = variable_get('patternbuilder_default_paragraphs_item_title', 'Pattern');
    $field['instance']['settings']['title_multiple'] = variable_get('patternbuilder_default_paragraphs_item_title_plural', 'Patterns');
    $field['instance']['settings']['default_edit_mode'] = 'closed';

    // Set allowed schemas.
    if (!empty($field['instance'])) {
      $items = NULL;
      if (isset($property->items)) {
        $resolved_items = pbi_utility::resolve_property_items($property);
        if (!empty($resolved_items->items)) {
          $items = &$resolved_items->items;
          if ($resolved_items->validOf == 'anyOf') {
            $field['base']['cardinality'] = -1;
          }
        }
      }
      elseif (isset($property->{'$ref'})) {
        $items = array($property);
      }

      if (isset($items)) {
        $settings = $this->build_widget($items);
        $field['instance']['settings'] = array_merge($field['instance']['settings'], $settings);
      }
    }

    return $field;
  }

  /**
   * Builds the allowed_bundles and bundle_weights for paragraphs references.
   *
   * Assumes all bundles are created before building the widget. If bundle
   * mapping does not exist, then allowed bundles is not set.
   *
   * @param array $items
   *   All of the items in allOf or oneOf.
   *
   * @return array
   *   Settings array for the widget.
   */
  private function build_widget(array $items) {
    $bundle_weight = 0;
    $settings = array();

    foreach ($items as $ref_comp) {
      $machine_name = patternbuilder_clean_schema_machine_name($ref_comp->{'$ref'});
      $component_map = patternbuilder_components_load($machine_name);
      if (!empty($component_map->bundle_name)) {
        $bundle_name = $component_map->bundle_name;
        $settings['allowed_bundles'][$bundle_name] = $bundle_name;
        $settings['bundle_weights'][$bundle_name] = $bundle_weight++;
      }
    }

    return $settings;
  }

}
