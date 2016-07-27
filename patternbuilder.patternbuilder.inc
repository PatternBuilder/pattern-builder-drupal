<?php

/**
 * @file
 * Pattern Builder integration for the patternbuilder module.
 */

/**
 * Implements hook_patternbuilder_pattern_status_info().
 */
function patternbuilder_patternbuilder_pattern_status_info() {
  $statuses = array();

  $statuses['active'] = array(
    'label' => 'Active',
    'weight' => -10,
  );

  $statuses['private'] = array(
    'label' => 'Private',
    'visible' => FALSE,
    'weight' => -5,
  );

  $statuses['deprecated'] = array(
    'label' => 'Deprecated',
    'visible' => FALSE,
    'creatable' => FALSE,
    'weight' => 0,
  );

  $statuses['inactive'] = array(
    'label' => 'Inactive',
    'import' => FALSE,
    'visible' => FALSE,
    'creatable' => FALSE,
    'weight' => 5,
  );

  return $statuses;
}
