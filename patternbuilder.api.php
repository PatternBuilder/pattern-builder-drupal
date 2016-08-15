<?php

/**
 * @file
 * Hooks provided by the patternbuilder module.
 */

/**
 * Provides pattern type information to the patternbuilder module.
 *
 * @return array
 *   An array keyed by the pattern type machine name.
 */
function hook_patternbuilder_pattern_types() {
  return array(
    // Key = pattern type machine name.
    'pattern' => array(
      // Un-translated human readable label for this pattern type.
      'label' => 'Pattern',
      // Define a prefix for the bundle or field name. Default is "pbi".
      'prefix' => 'mymod',
      // Indicate whether references to this schema should be resolved before
      // importing to fields.
      'resolve' => FALSE,
      // Indicate whether this schema should be imported to a single field.
      // If set to TRUE, then 'resolve' will also be set to TRUE. This is due
      // to the fact that field based schemas must be resolved.
      'field' => FALSE,
      // Claim a schema by machine name (ie file base name).
      'claim_by_name' => '_patternbuilder_importer_pattern_type_claim_by_name',
      // Claim a schema by inspecting the schema.
      'claim_by_schema' => '_patternbuilder_importer_pattern_type_claim_by_schema',
      // Define a weight to control the order of pattern type processing.
      // Default is 0.
      'weight' => 10,
    ),
  );
}

/**
 * Alter pattern type information for the patternbuilder module.
 *
 * @param array $types
 *   An array keyed by the pattern type machine name.
 */
function hook_patternbuilder_pattern_types_alter(&$types) {
  $types['pattern']['label'] = t('Generic Pattern');
}

/**
 * Example callback for hook_patternbuilder_pattern_types 'claim_by_name'.
 *
 * @param string $machine_name
 *   Pattern type machine name.
 *
 * @return bool
 *   TRUE to claim the pattern.
 */
function my_module_pattern_type_claim_by_name($machine_name) {
  return stripos($machine_name, 'pattern') !== FALSE;
}

/**
 * Example callback for hook_patternbuilder_pattern_types 'claim_by_schema'.
 *
 * @param string $machine_name
 *   Pattern type machine name.
 * @param object $schema
 *   JSON decoded schema object.
 *
 * @return bool
 *   TRUE to claim the pattern.
 */
function my_module_pattern_type_claim_by_schema($machine_name, $schema) {
  if (stripos($machine_name, 'pattern') !== FALSE) {
    if (isset($schema->myCustomTypeProperty) && $schema->myCustomTypeProperty == 'my_custom_type') {
      return TRUE;
    }
  }
}

/**
 * Defines pattern statuses.
 *
 * A pattern status controls the exposure of the pattern to Drupal. This
 * includes importing, visibility in the field widget, and available to use
 * on new nodes.
 *
 * The Pattern Builder module defines several statuses in its own
 * implementation of this hook,
 * patternbuilder_patternbuilder_pattern_status_info():
 * - Active: imported, visible, creatable.
 * - Private: imported, NOT visible, creatable.
 * - Deprecated: imported, NOT visible, NOT creatable.
 * - Inactive: NOT imported, NOT visible, NOT creatable.
 *
 * The status array structure is as follows:
 * - label: Required. The translatable label of the status, used in
 *   administrative interfaces.
 * - weight: Optional. integer weight of the status used for sorting lists of
 *   statuses. Defaults to 0.
 * - import: Optional. TRUE to import the pattern if the
 *   patternbuilder_importer module is enabled. Defaults to TRUE.
 * - visible: Optional. TRUE to display in the field widget's default list.
 *   If FALSE, the pattern must be explicitly selected in the field widget's
 *   settings. Defaults to TRUE.
 * - creatable: Optional. TRUE to allow selection on new entities.
 *   Defaults to TRUE.
 * - name: Internal only. The machine-name identifying the status using
 *   lowercase alphanumeric characters, -, and _.
 *   Defaults to the array key. Max 32 characters, if exceeded then the status
 *   is ignored.
 * - module: Internal only. The module that defined the status.
 *
 * @return array
 *   An array of pattern status arrays keyed by machine name.
 */
function hook_patternbuilder_pattern_status_info() {
  $statuses = array();

  $statuses['my_custom_status'] = array(
    'label' => t('Custom'),
    'import' => TRUE,
    'visible' => FALSE,
    'creatable' => FALSE,
  );

  return $statuses;
}

/**
 * Allows modules to alter the pattern status definitions of other modules.
 *
 * @param array $statuses
 *   An array of pattern statuses defined by enabled modules.
 *
 * @see hook_patternbuilder_pattern_status_info()
 */
function hook_patternbuilder_pattern_status_info_alter(&$statuses) {
  $statuses['active']['label'] = t('Public');
}
