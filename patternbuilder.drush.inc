<?php

/**
 * @file
 * Drush support for the PatternBuilder module.
 */

/**
 * Implements hook_drush_command().
 */
function patternbuilder_drush_command() {
  $items = array();
  $items['pb-warm-cache'] = array(
    'description' => 'Pre-load all known JSON objects, parse them and cache them.',
  );
  return $items;
}

/**
 * Drush call pb-warm-cache.
 */
function drush_patternbuilder_pb_warm_cache() {
  libraries_load('patternbuilder');
  $factory = new DrupalPatternBuilderSchema();
  $factory->warmCache();
}
