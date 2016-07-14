<?php

/**
 * @file
 * Views integration for the PatternBuilder module.
 */

/**
 * Implements hook_views_data().
 */
function patternbuilder_views_data() {
  $data = array();

  // Pattern Builder Components table.
  $data['patternbuilder_components']['table']['group']  = t('Pattern Builder Components');

  // Base table.
  $data['patternbuilder_components']['table']['base'] = array(
    'field' => 'id',
    'title' => t('Pattern Builder Components'),
    'weight' => -10,
    'access query tag' => 'patternbuilder_component_access',
    'defaults' => array(
      'field' => 'machine_name',
    ),
  );

  // Default join for other tables.
  $data['patternbuilder_components']['table']['join']  = array(
    'paragraphs_item' => array(
      'left_field' => 'bundle',
      'field' => 'bundle_name',
    ),
  );

  // Component id.
  $data['patternbuilder_components']['id'] = array(
    'title' => t('Component ID'),
    'help' => t('The patternbuilder component id.'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );

  $data['patternbuilder_components']['machine_name'] = array(
    'title' => t('Machine name'),
    'help' => t('The patternbuilder component machine name.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument',
    ),
  );

  $data['patternbuilder_components']['pattern_type'] = array(
    'title' => t('Pattern type'),
    'help' => t('The patternbuilder pattern type.'),
    'field' => array(
      'handler' => 'views_handler_field_machine_name',
      'options callback' => 'patternbuilder_pattern_types_labels',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_in_operator',
      'options callback' => 'patternbuilder_pattern_types_labels',
    ),
    'argument' => array(
      'handler' => 'patternbuilder_views_handler_argument_pattern_type',
    ),
  );

  $data['patternbuilder_components']['status'] = array(
    'title' => t('Pattern status'),
    'help' => t('The patternbuilder pattern status.'),
    'field' => array(
      'handler' => 'views_handler_field_machine_name',
      'options callback' => 'patternbuilder_pattern_status_get_label',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_in_operator',
      'options callback' => 'patternbuilder_pattern_status_get_label',
    ),
    'argument' => array(
      'handler' => 'patternbuilder_views_handler_argument_pattern_status',
    ),
  );

  $data['patternbuilder_components']['bundle_name'] = array(
    'title' => t('Paragraph bundle'),
    'help' => t('The patternbuilder paragraph bundle.'),
    'field' => array(
      'handler' => 'views_handler_field_machine_name',
      'options callback' => 'patternbuilder_paragraph_bundle_labels',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_in_operator',
      'options callback' => 'patternbuilder_paragraph_bundle_labels',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
    'relationship' => array(
      'handler' => 'views_handler_relationship',
      'base' => 'paragraphs_item',
      'base field' => 'bundle',
      'title' => t('Paragraph items'),
      'label' => t('Get the paragraph items associated with the patternbuilder component.'),
    ),
  );

  $data['patternbuilder_components']['field_name'] = array(
    'title' => t('Component field name'),
    'help' => t('The patternbuilder component field name.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument',
    ),
  );

  return $data;
}
