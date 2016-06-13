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
