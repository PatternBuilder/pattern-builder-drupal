<?php

/**
 * @file
 * Admin pages for patternbuilder.
 */

/**
 * Settings form.
 */
function patternbuilder_settings_form($form, &$form_state) {
  $form_id = __FUNCTION__;

  // Ensure this file is loaded when the form is built from cache.
  form_load_include($form_state, 'inc', 'patternbuilder', 'patternbuilder.admin');

  $form['dirs'] = array(
    '#type' => 'fieldset',
    '#title' => t('File directories'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Enter one directory per line. If the directory does not start with "/" or have a file scheme (public://), then it will be considered to be relative to the Drupal root directory.'),
  );

  $form['dirs']['examples'] = array(
    '#type' => 'item',
    '#title' => t('Examples'),
    '#markup' => theme('item_list', array(
      'items' => array(
        '"sites/all/libraries/pretty/schemas" (Drupal relative)',
        '"../pretty/schemas" (Drupal relative)',
        '"public://pretty/schemas" (used as defined)',
        '"file:///var/www-patterns/pretty/schemas" (used as defined)',
        '"/var/www-patterns/pretty/schemas" (used as defined)',
      ),
    )),
  );

  $schema_dirs = variable_get('patternbuilder_schema_dirs', array());
  if (empty($schema_dirs)) {
    $schema_dirs = array();
  }
  $form['dirs']['patternbuilder_schema_dirs'] = array(
    '#type' => 'textarea',
    '#title' => t('Schema directories'),
    '#rows' => 5,
    '#default_value' => implode("\n", $schema_dirs),
    '#description' => t('Enter the directories to scan for the schema files. The directory should contain schema files in the form of "machine_name.json". If duplicate file names are found, then the first schema is used.'),
  );

  $tpl_dirs = variable_get('patternbuilder_template_dirs', array());
  if (empty($tpl_dirs)) {
    $tpl_dirs = array();
  }
  $form['dirs']['patternbuilder_template_dirs'] = array(
    '#type' => 'textarea',
    '#title' => t('Template directories'),
    '#rows' => 5,
    '#default_value' => implode("\n", $tpl_dirs),
    '#description' => t('Enter the directories to scan for TWIG template files.'),
  );

  $form['grid'] = array(
    '#type' => 'fieldset',
    '#title' => t('Grid layout on the Edit Form'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Configure settings for the grid layout on the edit form.'),
  );

  $grid_settings = patternbuilder_grid_layout_global_settings();
  $form['grid']['patternbuilder_grid_row_class'] = array(
    '#type' => 'textfield',
    '#title' => t('Row class'),
    '#description' => t('The class for the row container in the grid layout.'),
    '#default_value' => $grid_settings['row_class'],
  );

  $form['grid']['patternbuilder_grid_col_class_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Column class prefix'),
    '#default_value' => $grid_settings['col_class_prefix'],
    '#description' => t('The number of columns will be appended to the column class prefix. Examples: !examples', array(
      '!examples' => theme('item_list', array(
        'items' => array(
          '"grid-columns-" results in "grid-columns-4"',
          '"col col-" results in "col col-4"',
        ),
      )),
    )),
  );

  // Pattern Status.
  $form['pattern_status'] = array(
    '#type' => 'fieldset',
    '#title' => t('Pattern Status'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Configure pattern statuses.'),
  );

  $status_options = patternbuilder_pattern_status_options_list();
  $default_status = patternbuilder_pattern_status_default();
  $default_status_key = isset($default_status['name']) ? $default_status['name'] : NULL;

  $form['pattern_status']['patternbuilder_pattern_status_ui_access'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow status to be updated via the UI'),
    '#description' => t('If enabled then the pattern status can be updated on the paragraphs Pattern page ("admin/structure/paragraphs/example-pattern/patternbuilder").'),
    '#default_value' => variable_get('patternbuilder_pattern_status_ui_access', TRUE),
  );

  $form['pattern_status']['patternbuilder_pattern_status_default'] = array(
    '#type' => 'select',
    '#title' => t('Default pattern status'),
    '#description' => t('The default pattern status is used when creating new patterns that do not have a defined status. The Pattern Builder importer sets the status based on the value of the JSON schema property "status". The JSON value should be the machine name of the status. Refer to the table below for information on the available statuses.'),
    '#default_value' => $default_status_key,
    '#options' => $status_options,
  );

  // Display all available statuses.
  $form['pattern_status']['patternbuilder_pattern_statuses'] = array(
    '#type' => 'item',
    '#title' => t('Available Pattern Statuses'),
    '#markup' => theme('patternbuilder_pattern_statuses', array(
      'statuses' => patternbuilder_pattern_statuses(),
    )),
  );

  $form['#validate'][] = 'patternbuilder_settings_form_validate';
  $form = system_settings_form($form);
  $form['#submit'][] = 'patternbuilder_settings_form_submit';

  return $form;
}

/**
 * Settings form validate callback.
 */
function patternbuilder_settings_form_validate($form, &$form_state) {
  $values = &$form_state['values'];
  $dir_vars = array(
    'patternbuilder_schema_dirs',
    'patternbuilder_template_dirs',
  );

  // Parse variables to arrays.
  foreach ($dir_vars as $name) {
    if (isset($values[$name])) {
      $new_value = preg_split('@\r\n|\n|\r@', $values[$name], -1, PREG_SPLIT_NO_EMPTY);
      $new_value = array_map('trim', $new_value);
      $new_value = array_filter($new_value, 'strlen');
      if (empty($new_value)) {
        $new_value = array();
      }

      form_set_value($form['dirs'][$name], $new_value, $form_state);
    }
  }
}

/**
 * Settings form submit callback.
 *
 * This is triggered after the variables are saved in system_settings_form().
 */
function patternbuilder_settings_form_submit($form, &$form_state) {
  // Invalidate all patternbuilder cache for changes to be active.
  patternbuilder_invalidate_cache();
}

/**
 * Pattern form of a paragraph bundle.
 */
function patternbuilder_admin_bundle_form($form, &$form_state, $bundle = NULL) {
  // Ensure this include file is loaded when the form is rebuilt from the cache.
  $form_state['build_info']['files']['form'] = drupal_get_path('module', 'patternbuilder') . '/patternbuilder.admin.inc';

  $component = patternbuilder_get_bundle_component($bundle->bundle);
  $form_state['patternbuilder_component'] = $component;

  $info_labels = array(
    'id' => t('Internal ID'),
    'machine_name' => t('Machine name'),
    'pattern_type' => t('Type'),
    'schema_path' => t('JSON schema path'),
    'template_path' => t('Default template path')
  );

  $pattern_type_labels = patternbuilder_pattern_types_labels();

  $info_items = array();

  // Stored component mapping info.
  foreach ($info_labels as $info_key => $info_label) {
    if (isset($component->{$info_key})) {
      if ($info_key == 'pattern_type' && !empty($pattern_type_labels[$component->{$info_key}]) && $pattern_type_labels[$component->{$info_key}] != $component->{$info_key}) {
        $info_value = t('@human_pattern_type_label (@machine_pattern_type_label)', array(
          '@human_pattern_type_label' => $pattern_type_labels[$component->{$info_key}],
          '@machine_pattern_type_label' => $component->{$info_key},
        ));
      }
      else {
        $info_value = check_plain($component->{$info_key});
      }
      $info_items[$info_key] = array(
        '#type' => 'item',
        '#field_prefix' => $info_label . ':',
        '#markup' => $info_value,
      );
    }
  }

  // JSON Schema.
  $schemas = patternbuilder_get_schemas();
  if (!empty($schemas[$component->machine_name])) {
    $schema_path = $schemas[$component->machine_name];
    if (strpos($schema_path, 'file://') === 0) {
      $schema_path = preg_replace('@^file\:\/\/@', '', $schema_path);
    }
    $info_items['schema_path'] = array(
      '#type' => 'item',
      '#field_prefix' => $info_labels['schema_path'] . ':',
      '#markup' => check_plain($schema_path),
    );

    // Get Twig file name to find.
    $template_filename = '';
    $factory = patternbuilder_get();
    $pattern = $factory->load($component->machine_name);
    if ($pattern && method_exists($pattern, 'getTheme')) {
      $template_filename = $pattern->getTheme();
    }
    if (empty($template_filename)) {
      $template_filename = $component->machine_name . '.twig';
    }

    // Find TWIG template path.
    $template_paths = patternbuilder_get_template_paths();
    $twig_loader = new Twig_Loader_Filesystem($template_paths);

    $template_path = '';
    try {
      $template_path = $twig_loader->getCacheKey($template_filename);
    }
    catch (Twig_Error_Loader $e) {
    }

    if (empty($template_path)) {
      $template_path = t('No file found.');
    }
    else {
      $template_path = check_plain($template_path);
    }
    $info_items['template_path'] = array(
      '#type' => 'item',
      '#field_prefix' => $info_labels['template_path'] . ':',
      '#markup' => $template_path,
    );
  }

  if ($info_items) {
    $form['pattern_info'] = array(
      '#type' => 'fieldset',
      '#title' => t('Pattern details'),
    );
    $form['pattern_info'] += $info_items;
  }

  // Pattern status.
  $form['status_wrapper'] = array(
    '#type' => 'fieldset',
    '#title' => t('Pattern Status'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Configure the pattern status.'),
  );

  $ui_access = array(
    'status' => variable_get('patternbuilder_pattern_status_ui_access', TRUE),
  );

  $status_options = patternbuilder_pattern_status_options_list();
  $default_status = patternbuilder_pattern_status_default();
  $default_status_key = isset($default_status['name']) ? $default_status['name'] : NULL;
  $form['status_wrapper']['status'] = array(
    '#type' => 'select',
    '#title' => t('Pattern status'),
    '#description' => t('The Pattern Builder importer sets the status based on the value of the JSON schema property "status". The JSON value should be the machine name of the status. Refer to the table below for information on the available statuses. '),
    '#default_value' => isset($component->status) ? $component->status : $default_status_key,
    '#options' => $status_options,
    '#disabled' => !$ui_access['status'],
  );

  $form['status_wrapper']['patternbuilder_pattern_statuses'] = array(
    '#type' => 'item',
    '#title' => t('Available Pattern Statuses'),
    '#markup' => theme('patternbuilder_pattern_statuses', array(
      'statuses' => patternbuilder_pattern_statuses(),
    )),
  );

  // Form actions.
  if (array_filter($ui_access)) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save pattern'),
      '#weight' => 50,
    );
  }

  return $form;
}

/**
 * Submit handler for the pattern form of a paragraph bundle.
 */
function patternbuilder_admin_bundle_form_submit($form, &$form_state) {
  // Update the component if the status is set.
  if (!empty($form_state['values']['status']) && !empty($form_state['patternbuilder_component'])) {
    $form_state['patternbuilder_component']->status = $form_state['values']['status'];
    patternbuilder_component_save($form_state['patternbuilder_component']);
    drupal_set_message(t('The pattern has been updated.'));
  }
  else {
    drupal_set_message(t('There were no changes to the pattern.'));
  }
}

/**
 * Override page callback for paragraphs bundle list.
 */
function patternbuilder_paragraphs_admin_bundle_overview() {
  module_load_include('inc', 'paragraphs', 'paragraphs.admin');
  $page = paragraphs_admin_bundle_overview();

  if (!empty($page['paragraphs_bundle_table']['#rows'])) {
    // Expand the header column to accomodate the new column.
    if (isset($page['paragraphs_bundle_table']['#header'][1]['colspan'])) {
      $page['paragraphs_bundle_table']['#header'][1]['colspan'] = 5;
    }

    // Get components.
    $available_bundles = paragraphs_bundle_load();
    $bundle_components = patternbuilder_get_bundle_component_map();

    // Sort the table.
    $bundle_labels = array();
    foreach ($available_bundles as $bundle_name => $bundle_info) {
      $bundle_labels[$bundle_name] = isset($bundle_info->name) ? $bundle_info->name : $bundle_name;
    }
    uasort($bundle_labels, 'strnatcasecmp');

    $sorted_keys = array_keys($bundle_labels);
    $pattern_keys = array_intersect($sorted_keys, array_keys($bundle_components));
    $non_patterns_keys = array_diff($sorted_keys, $pattern_keys);
    $row_keys = array_keys($page['paragraphs_bundle_table']['#rows']);
    $sorted_row_keys = array_merge($pattern_keys, $non_patterns_keys);
    $sorted_row_keys = array_merge($sorted_row_keys, array_diff($row_keys, $sorted_row_keys));

    // Alter bundle rows.
    $rows = $page['paragraphs_bundle_table']['#rows'];
    $new_rows = array();
    foreach ($sorted_row_keys as $bundle_name) {
      if (!isset($rows[$bundle_name])) {
        continue;
      }

      $row = $rows[$bundle_name];
      $row_attributes = array(
        'class' => array(drupal_html_class($bundle_name)),
      );
      $bundle_url_string = strtr($bundle_name, array('_' => '-'));

      // If bundle is a pattern ...
      if (!empty($bundle_components[$bundle_name])) {
        $component = $bundle_components[$bundle_name];

        // Pattern info in bundle label column.
        if (!empty($row[0]['data'])) {
          $row[0]['data'] .= '<div class="patternbuilder-label-info">';
          $row[0]['data'] .= t('@type: @name (@status)', array(
            '@name' => $component->machine_name,
            '@type' => !empty($component->pattern_type) ? patternbuilder_pattern_types_labels($component->pattern_type) : '',
            '@status' => !empty($component->status) ? patternbuilder_pattern_status_get_label($component->status) : '',
          ));
          $row[0]['data'] .= '</div>';
        }

        // Pattern form link column.
        $row[] = array(
          'data' => l(t('manage pattern'), 'admin/structure/paragraphs/' . $bundle_url_string . '/patternbuilder')
        );
      }
      else {
        // Empty string for the Pattern form link column.
        $row[] = array('data' => '&nbsp');
      }

      // Make this a complex row.
      $row = array('data' => $row) + $row_attributes;

      // Set new row.
      $new_rows[$bundle_name] = $row;
    }

    if ($new_rows) {
      $page['paragraphs_bundle_table']['#rows'] = $new_rows;
    }
  }

  return $page;
}
