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
    'label' => t('Active'),
    'weight' => -10,
  );

  $statuses['private'] = array(
    'label' => t('Private'),
    'visible' => FALSE,
    'weight' => -5,
  );

  $statuses['deprecated'] = array(
    'label' => t('Deprecated'),
    'visible' => FALSE,
    'creatable' => FALSE,
    'weight' => 0,
  );

  $statuses['inactive'] = array(
    'label' => t('Inactive'),
    'import' => FALSE,
    'visible' => FALSE,
    'creatable' => FALSE,
    'weight' => 5,
  );

  return $statuses;
}
