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
      // Human readable label for this pattern type.
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
 * Example callback for hook_patternbuilder_pattern_types 'claim_by_name'.
 *
 * @param string $machine_name
 *   Pattern type machine name.
 *
 * @return bool
 *   TRUE to claim the pattern.
 */
function MY_MODULE_pattern_type_claim_by_name($machine_name) {
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
function MY_MODULE_pattern_type_claim_by_schema($machine_name, $schema) {
  if (stripos($machine_name, 'pattern') !== FALSE) {
    if (isset($schema->myCustomTypeProperty) && $schema->myCustomTypeProperty == 'my_custom_type') {
      return TRUE;
    }
  }
}
