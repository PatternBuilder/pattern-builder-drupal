<?php

/**
 * @file
 * Default theme functions for paragraphs.
 */

/**
 * Process variables for patternbuilder-items.tpl.php.
 */
function template_preprocess_patternbuilder_items(&$vars) {
  $vars['view_mode'] = $vars['element']['#view_mode'];
  $vars['field_name'] = $vars['element']['#field_name'];

  // Helpful $content variable for templates.
  $vars['content'] = array();
  foreach (element_children($vars['element']) as $key) {
    $vars['content'][$key] = $vars['element'][$key];
  }

  // Build classes.
  $vars['classes_array'][] = drupal_html_class('patternbuilder_items__' . $vars['element']['#field_name']);

  // Add theme suggestions.
  $vars['theme_hook_suggestions'][] = 'patternbuilder_items__' . $vars['element']['#field_name'];
}

/**
 * Process variables for patternbuilder-item.tpl.php.
 */
function template_preprocess_patternbuilder_item(&$vars) {
  if (isset($vars['elements']['#entity_type']) && $vars['elements']['#entity_type'] != 'paragraphs_item') {
    return;
  }
  $entity_type = $vars['elements']['#entity_type'];
  $vars['entity_type'] = $entity_type;

  $langcode = !empty($vars['elements']['#language']) ? $vars['elements']['#language'] : LANGUAGE_NONE;
  $vars['language'] = $langcode;

  $entity = $vars['elements']['#entity'];
  $vars['entity'] = $entity;

  $entity_info = entity_get_info($entity_type);
  list($entity_id, $revision_id , $bundle) = entity_extract_ids($entity_type, $entity);
  $vars['bundle'] = $bundle;
  $vars['entity_id'] = $entity_id;
  $vars['revision_id'] = $revision_id;
  $vars['pattern_component'] = patternbuilder_get_bundle_component($bundle);

  $view_mode = 'full';
  if (!empty($vars['elements']['#view_mode']) && isset($entity_info['view modes'][$vars['elements']['#view_mode']])) {
    $view_mode = $vars['elements']['#view_mode'];
  }
  $vars['view_mode'] = $view_mode;

  // Build the content.
  if (!empty($vars['pattern_component'])) {
    // Patterns.
    $vars['is_pattern'] = TRUE;
    $vars['content'] = patternbuilder_render_schema($entity, $view_mode, $langcode);
  }
  else {
    // Non-patterns.
    $vars['is_pattern'] = FALSE;
    $vars['content'] = $entity->view($view_mode);
  }

  // Build css classes.
  $vars['classes_array'][] = drupal_html_class('patternbuilder-item--' . $bundle);

  // Add theme suggestions.
  $vars['theme_hook_suggestions'][] = 'patternbuilder_item__' . $bundle;
  if ($entity_id) {
    $vars['theme_hook_suggestions'][] = 'patternbuilder_item__' . $entity_id;
  }
}

/**
 * Render a paragraph with the defined schema.
 *
 * @param ParagraphsItemEntity $entity
 *   The paragraph item entity object used to stored the schema input data.
 * @param string $view_mode
 *   The view mode to render.
 * @param string $langcode
 *   The language to render.
 *
 * @return array
 *   A renderable array.
 */
function patternbuilder_render_schema(ParagraphsItemEntity $entity, $view_mode = 'full', $langcode = NULL) {
  $entity_type = 'paragraphs_item';

  $builder = new DrupalPatternBuilder($entity_type, $entity);
  if ($builder->canBuild()) {
    try {
      $contents = $builder->render($view_mode);
      if ($contents) {
        return array(
          'build' => array('#markup' => $contents),
        );
      }
    }
    catch (Exception $e) {
      watchdog('patternbuilder', 'An error occurred while rendering the @bundle paragraph item (id:@id, revision:@vid) on @host_entity_type (id:@host_id, revision:@host_vid): @error', array(
        '@id' => $entity->item_id,
        '@vid' => $entity->revision_id,
        '@bundle' => $entity->bundle(),
        '@host_id' => @$entity->hostEntityId(),
        '@host_vid' => @$entity->hostEntityRevisionId(),
        '@host_entity_type' => @$entity->hostEntityType(),
        '@error' => $e->getMessage(),
      ), WATCHDOG_ERROR);
    }
  }
}

/**
 * Theme a list of pattern statuses.
 *
 * @param array $vars
 *   The theme variables.
 *
 * @return string
 *   HTML markup to display the list of statuses.
 */
function theme_patternbuilder_pattern_statuses($vars) {
  if (empty($vars['statuses'])) {
    return '';
  }

  $statuses_headers = array(
    t('Label (Machine name)'), t('Module'), t('Imported'), t('Visible in widget'), t('Creatable'),
  );
  $status_booleans = array('import', 'visible', 'creatable');
  $statuses_rows = array();
  foreach ($vars['statuses'] as $status_key => $status) {
    $status_row = array();
    $status_row[] = t('@label (@name)', array(
      '@label' => t($status['label']),
      '@name' => $status_key,
    ));
    $status_row[] = check_plain($status['module']);

    foreach ($status_booleans as $status_boolean_key) {
      $status_row[] = !empty($status[$status_boolean_key]) ? t('Yes') : t('No');
    }

    $statuses_rows[] = $status_row;
  }

  if ($statuses_rows) {
    return theme('table', array(
      'header' => $statuses_headers,
      'rows' => $statuses_rows,
    ));
  }
  return '';
}
