<?php

/**
 * @file
 * Class to handle Boolean field display instances.
 *
 * File is not namespaced in order to work with D7 class loading.
 */

/**
 * Class to handle Boolean field display instances.
 */
class DrupalPatternBuilderDisplayInstanceBoolean extends DrupalPatternBuilderDisplayInstance {

  protected $booleanOffValue = 0;
  protected $booleanOnValue = 1;

  /**
   * {@inheritdoc}
   */
  public function __construct($entity, array $field_instance, $field_display = 'default', $langcode = NULL) {
    parent::__construct($entity, $field_instance, $field_display, $langcode);
    if (isset($this->display)) {
      $field = field_info_field($this->fieldName);
      $allowed_values = list_allowed_values($field, $this->instance, $this->entityType, $this->entity);
      $allowed_keys = array_keys($allowed_values);
      if (count($allowed_keys) == 2) {
        $this->booleanOffValue = array_shift($allowed_keys);
        $this->booleanOnValue = array_shift($allowed_keys);
      }
    }
  }

  /**
   * Renders each field item separately to avoid wrapping markup.
   *
   * @return array
   *   An array of rendered field items.
   */
  protected function viewField() {
    $items = DrupalPatternBuilder::fieldGetItems($this->entityType, $this->entity, $this->fieldName, $this->language);
    if (empty($items)) {
      return array();
    }

    $renders = array();
    foreach ($items as $delta => $item) {
      if (isset($item['value']) && $item['value'] == $this->booleanOnValue) {
        $renders[$delta] = TRUE;
      }
      else {
        $renders[$delta] = FALSE;
      }
    }

    return $renders;
  }

}
