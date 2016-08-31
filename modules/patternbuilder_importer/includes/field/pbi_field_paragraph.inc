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
  public static function settings_defaults() {
    $defaults = array(
      'instance__title' => 'Pattern',
      'instance__title_multiple' => 'Patterns',
      'instance__default_edit_mode' => 'closed',
      'widget__type' => 'patternbuilder_paragraphs_embed',
    );

    $defaults += parent::settings_defaults();
    return $defaults;
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_form() {
    $form = parent::settings_form();
    $storage_id = static::settings_storage_id();
    $classname = get_called_class();
    $field_type = static::global_field_type();

    // Extract the form from the paragraphs instance form.
    $instance_form = array();
    $handler_instance_settings = static::handler_instance_settings();
    if ($handler_instance_settings) {
      $fake_field = array();
      $fake_instance = array('settings' => $handler_instance_settings);
      $paragraphs_instance_form = module_invoke($field_type, 'field_instance_settings_form', $fake_field, $fake_instance);
      if ($paragraphs_instance_form) {
        // Cleanup un-used elements.
        unset($paragraphs_instance_form['fieldset']);
        unset($paragraphs_instance_form['allowed_bundles_table']);

        // Re-key the form.
        foreach (element_children($paragraphs_instance_form) as $instance_setting_name) {
          $new_setting_name = 'instance__' . $instance_setting_name;
          $instance_form[$new_setting_name] = $paragraphs_instance_form[$instance_setting_name];
          if (isset($paragraphs_instance_form[$instance_setting_name]['#states'])) {
            $states = $paragraphs_instance_form[$instance_setting_name]['#states'];
            foreach ($states as $state => $state_conditions) {
              $new_state_conditions = array();
              foreach ($state_conditions as $state_condition_selector => $state_condition) {
                $new_state_condition_selector = preg_replace('@instance\[settings\]\[([^\]]+)\]@', $storage_id . '[instance__$1]', $state_condition_selector);
                $new_state_conditions[$new_state_condition_selector] = $state_condition;
              }
              $states[$state] = $new_state_conditions;
            }
            unset($state_conditions);

            $instance_form[$new_setting_name]['#states'] = $states;
          }
        }
      }
    }

    $widget_form = array();
    $widget_settings = static::handler_widget_settings();
    if (isset($widget_settings['type'])) {
      $widget_type_options = static::field_widget_type_options($field_type);
      if ($widget_type_options) {
        $widget_form['widget__type'] = array(
          '#type' => 'select',
          '#title' => t('Widget type'),
          '#required' => TRUE,
          '#options' => $widget_type_options,
          '#default_value' => $widget_settings['type'],
          '#description' => t('The type of form element you would like to present to the user when creating this field.'),
        );
      }
    }

    if ($instance_form || $widget_form) {
      $form[$storage_id] = array(
        '#type' => 'fieldset',
        '#title' => $classname,
        '#description' => t('Settings for paragraph fields within a pattern.'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#tree' => TRUE,
      );
      $form[$storage_id] += $instance_form;
      $form[$storage_id] += $widget_form;
    }

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_default_field_formatter($property, array $base, array $instance) {
    $formatter = parent::determine_default_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      $formatter = array(
        'type' => 'patternbuilder_view',
        'label' => 'hidden',
        'settings' => array(
          'view_mode' => 'full',
        ),
      );
    }

    return $formatter;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_preview_field_formatter($property, array $base, array $instance) {
    $formatter = parent::determine_preview_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      $formatter = array(
        'type' => 'paragraphs_view',
        'label' => 'hidden',
        'settings' => array(
          'view_mode' => 'paragraphs_editor_preview',
        ),
      );
    }

    return $formatter;
  }

  /**
   * {@inheritdoc}
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $field = parent::preprocess_field($name, $property, $required_properties, $weight);

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
        $bundle_settings = $this->build_paragraphs_bundle_settings($items, $field);
        $field['instance']['settings'] = array_merge($field['instance']['settings'], $bundle_settings);
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
   * @param array $import
   *   Combined array of the imported field base and instance.
   *
   * @return array
   *   Settings array for the widget.
   */
  private function build_paragraphs_bundle_settings(array $items, array $import = array()) {
    $bundle_weight = 0;
    $bundle_settings = array();

    foreach ($items as $ref_comp) {
      $machine_name = patternbuilder_clean_schema_machine_name($ref_comp->{'$ref'});
      $component = patternbuilder_components_load($machine_name);
      if (!empty($component->bundle_name)) {
        $bundle_name = $component->bundle_name;

        // Warn about invalid references based on the status.
        // Only a warning to allow the field settings to be saved with any
        // scheme. The patternbuilder field widget will filter out statuses
        // that should not be shown in the options list.
        if (!empty($component->status)) {
          // Log error for references to non-imported schemas.
          if (!patternbuilder_component_is_imported($component)) {
            watchdog('patternbuilder_importer', 'The paragraphs field @field_name references a schema that is not imported due to the status "@status", schema: @machine_name', array(
              '@field_name' => !empty($import['base']['field_name']) ? '"' . $import['base']['field_name'] . '"' : '',
              '@machine_name' => $machine_name,
              '@status' => patternbuilder_pattern_status_get_label($component->status),
            ), WATCHDOG_WARNING);
          }

          // Log warning for references to non-creatable schemas.
          if (!patternbuilder_component_is_creatable($component)) {
            watchdog('patternbuilder_importer', 'The paragraphs field @field_name references a schema that is not creatable due to the status "@status", schema: @machine_name', array(
              '@field_name' => !empty($import['base']['field_name']) ? '"' . $import['base']['field_name'] . '"' : '',
              '@machine_name' => $machine_name,
              '@status' => patternbuilder_pattern_status_get_label($component->status),
            ), WATCHDOG_WARNING);
          }
        }

        // Update widget settings.
        $bundle_settings['allowed_bundles'][$bundle_name] = $bundle_name;
        $bundle_settings['bundle_weights'][$bundle_name] = $bundle_weight++;
      }
    }

    return $bundle_settings;
  }

}
