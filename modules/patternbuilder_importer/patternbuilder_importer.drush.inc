<?php

/**
 * @file
 * Drush interface for patternbuilder_importer.
 */

/**
 * Implements hook_drush_command().
 */
function patternbuilder_importer_drush_command() {
  $items = array();

  $items['pbi'] = array(
    'description' => dt('Import pattern schemas as reusable paragraph entities.'),
    'aliases' => array('pbi'),
    'arguments' => array(
      'name' => 'The machine name of the schema. Example: "band"',
    ),
    'options' => array(
      'all' => 'Forces the import of all schemas and ignores any name arguments',
      'type' => 'Imports schemas for a specific pattern type.',
    ),
    'examples' => array(
      'drush pbi' => 'Import all schemas.',
      'drush pbi --all' => 'Force the import of all schemas even if specific schemas are defined.',
      'drush pbi band card' => 'Import only the "band" and "card" schemas.',
      'drush pbi --type=layout' => 'Import only the schemas for the "layout" pattern type.',
    ),
  );

  $items['pbi-remove'] = array(
    'description' => dt('Remove imported pattern schemas.'),
    'aliases' => array('pbi-remove'),
    'arguments' => array(
      'name' => 'The machine name of the schema. Example: "band"',
    ),
    'options' => array(
      'all' => 'Removes all schemas and ignores any name arguments. This must be set to remove all schemas.',
      'type' => 'Removes schemas for a specific pattern type.',
    ),
    'examples' => array(
      'drush pbi-remove' => 'Nothing is removed.',
      'drush pbi-remove --all' => 'Removes all schemas.',
      'drush pbi-remove band card' => 'Removes only the "band" and "card" schemas.',
      'drush pbi-remove --type=layout' => 'Removes only the schemas for the "layout" pattern type.',
    ),
  );

  return $items;
}

/**
 * Import a specific schema or all if specified.
 */
function drush_patternbuilder_importer_pbi() {
  $is_all = drush_get_option('all');

  if ($is_all) {
    patternbuilder_importer_import_schemas();
  }
  else {
    $schema_names = func_get_args();
    $schema_names = !empty($schema_names) ? $schema_names : array();

    $type = drush_get_option('type');
    $pattern_types = !empty($type) ? array($type) : array();

    patternbuilder_importer_import_schemas($schema_names, $pattern_types);
  }
}

/**
 * Remove a specific schema or all if specified.
 */
function drush_patternbuilder_importer_pbi_remove() {
  $patterns_to_remove = array();
  $bundles_available = paragraphs_bundle_load();
  if (empty($bundles_available)) {
    drush_print(dt('There are no bundles to remove.'));
    return;
  }

  $is_all = drush_get_option('all');
  if ($is_all) {
    $patterns_to_remove = patternbuilder_get_bundle_component_map();
  }
  elseif ($args = func_get_args()) {
    $components = patternbuilder_components_load();
    $arg_components = array_intersect_key($components, array_combine($args, $args));
    if ($arg_components) {
      foreach ($arg_components as $machine_name => $component) {
        if (!empty($component->bundle_name)) {
          $patterns_to_remove[$component->bundle_name] = $component;
        }
      }
    }
  }

  // Filter by pattern type.
  $pattern_type = drush_get_option('type');
  if (!empty($pattern_type)) {
    foreach (array_keys($patterns_to_remove) as $bundle_name) {
      if (isset($patterns_to_remove[$bundle_name])) {
        $component = $patterns_to_remove[$bundle_name];
        if (empty($component->pattern_type) || $component->pattern_type != $pattern_type) {
          unset($patterns_to_remove[$bundle_name]);
        }
      }
    }
  }

  // Limit to bundles installed.
  $patterns_to_remove = array_intersect_key($patterns_to_remove, $bundles_available);

  if ($patterns_to_remove) {
    // Remove bundle.
    foreach ($patterns_to_remove as $bundle_name => $component) {
      paragraphs_bundle_delete($bundle_name);
      drush_log(dt('@component (bundle: @bundle) removed.', array(
        '@component' => $component->machine_name,
        '@bundle' => $bundle_name,
      )), 'ok');
    }

    // Cleanup fields.
    field_purge_batch(10);
  }
  else {
    drush_print(dt('There are no imported bundles to remove.'));
  }
}
