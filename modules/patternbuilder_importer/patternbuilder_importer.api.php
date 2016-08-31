<?php

/**
 * @file
 * Hooks provided by the patternbuilder_importer module.
 */

/**
 * Alter the imported field instance settings.
 *
 * This is used to make any customizations to the instance settings before
 * extending field import handlers and before the
 * hook_patternbuilder_importer_field_import_alter().
 *
 * @param array $instance_settings
 *   An array of field instance settings.
 * @param array $context
 *   An array with the following:
 *   - 'base' array: The importer generated field base array.
 *   - 'instance' array: The importer generated field instance array.
 *   - 'property' object: The schema property.
 *   - 'handler' object: The instance of the field import class.
 */
function hook_patternbuilder_importer_field_instance_settings_alter(&$instance_settings, $context) {
  if (isset($context['property']->specialProperty) && $context['property']->specialProperty == 1) {
    $instance_settings['my_module']['special'] = "only_one";
  }
}

/**
 * Provides available formatters for a given schema property.
 *
 * This is used by the importer to determine the field's default formatter.
 * The formatters returned by all the modules are sorted by 'pb_weight' and
 * then the top formatter is used.
 *
 * @param array $context
 *   An array with the following:
 *   - 'property' object: The schema property.
 *   - 'field_base' array: The importer generated field base array.
 *   - 'field_instance' array: The importer generated field instance array.
 *
 * @return array
 *   An array of Drupal formatter arrays.
 */
function hook_patternbuilder_importer_property_formatter(array $context) {
  if (isset($context['property']->formatter) && $context['property']->formatter == 'pretty_callout' && $context['field_base']['type'] == 'text_long') {
    return array(
      array(
        // Type is required.
        'type' => 'my_module_pretty_callout',
        // All formatter display values are allowed.
        'label' => 'hidden',
        'settings' => array(
          'size' => 'full',
        ),
        // Drupal field display weight.
        'weight' => 5,
        // Weight for hook_patternbuilder_default_formatter().
        // The formatters returned by all the modules are sorted by 'pb_weight'
        // and then the top formatter is used.
        'pb_weight'  => -10,
      ),
    );
  }
}

/**
 * Alter the available formatters for a given schema property.
 *
 * See hook_patternbuilder_importer_property_formatter().
 *
 * @param array $formatters
 *   The formatters found in hook_patternbuilder_importer_property_formatter().
 * @param array $context
 *   An array with the following:
 *   - 'property' object: The schema property.
 *   - 'field_base' array: The importer generated field base array.
 *   - 'field_instance' array: The importer generated field instance array.
 */
function hook_patternbuilder_importer_property_formatter_alter(array &$formatters, array $context) {
  unset($formatters['bad_module::0']);
}

/**
 * Provides available formatters for a given schema property.
 *
 * This is used by the importer to determine the field's default formatter.
 * The formatters returned by all the modules are sorted by 'pb_weight' and
 * then the top formatter is used.
 *
 * @param array $context
 *   An array with the following:
 *   - 'property' object: The schema property.
 *   - 'field_base' array: The importer generated field base array.
 *   - 'field_instance' array: The importer generated field instance array.
 *
 * @return array
 *   An array of Drupal formatter arrays.
 */
function hook_patternbuilder_importer_property_preview_formatter(array $context) {
  $text_types = array('text', 'text_long', 'text_with_summary');
  if (in_array($context['field_base']['type'], $text_types, TRUE)) {
    return array(
      array(
        // Type is required.
        'type' => 'smart_trim_format',
        // All formatter display values are allowed.
        'label' => 'hidden',
        'module' => 'smart_trim',
        'settings' => array(
          'more_link' => 0,
          'more_text' => 'Read more',
          'summary_handler' => 'trim',
          'trim_type' => 'chars',
          'trim_length' => 140,
          'trim_link' => 0,
          'trim_preserve_tags' => '',
          'trim_suffix' => '',
          'trim_options' => array(
            'smart_boundaries' => 'smart_boundaries',
            'text' => 'text',
          ),
        ),

        // Drupal field display weight.
        'weight' => 5,
        // Weight for hook_patternbuilder_importer_property_preview_formatter().
        // The formatters returned by all the modules are sorted by 'pb_weight'
        // and then the top formatter is used.
        'pb_weight'  => -10,
      ),
    );
  }
}

/**
 * Alter the available preview formatters for a given schema property.
 *
 * See hook_patternbuilder_importer_property_preview_formatter().
 *
 * @param array $formatters
 *   The formatters found in
 *   hook_patternbuilder_importer_property_preview_formatter().
 * @param array $context
 *   An array with the following:
 *   - 'property' object: The schema property.
 *   - 'field_base' array: The importer generated field base array.
 *   - 'field_instance' array: The importer generated field instance array.
 */
function hook_patternbuilder_importer_property_preview_formatter_alter(array &$formatters, array $context) {
  unset($formatters['bad_module::0']);
}

/**
 * Alter the default field value.
 *
 * @param mixed $value
 *   The value of the field's value key property.
 * @param array $context
 *   An array with the following:
 *   - 'property' object: The schema property.
 *   - 'field_base' array: The importer generated field base array.
 *   - 'field_instance' array: The importer generated field instance array.
 *   - 'allowed_values' array: An array of allowed values.
 */
function hook_patternbuilder_importer_property_default_value_alter(&$value, array $context) {
  $value = 'my_default_value';
}

/**
 * Alter the imported field base and instance information before save.
 *
 * This is used to make any customizations to the field base, instance, widget
 * and formatter.
 *
 * @param array $field_import
 *   An array with the following:
 *   - 'base' array: The importer generated field base array.
 *   - 'instance' array: The importer generated field instance array.
 * @param array $context
 *   An array with the following:
 *   - 'property' object: The schema property.
 *   - 'handler' object: The instance of the field import class.
 */
function hook_patternbuilder_importer_field_import_alter(&$field_import, array $context) {
  if ($field_import['base']['type'] === 'link_field') {
    // Set url as optional.
    $field_import['instance']['settings']['url'] = 1;
  }
}
