<?php

/**
 * @file
 * Definition of patternbuilder_views_handler_argument_pattern_type.
 */

/**
 * Argument handler to accept a pattern type.
 */
class patternbuilder_views_handler_argument_pattern_type extends views_handler_argument_string {
  /**
   * {@inheritdoc}
   */
  public function construct() {
    parent::construct('pattern_type');
  }

  /**
   * Override the behavior of summary_name().
   */
  public function summary_name($data) {
    return check_plain($this->pattern_type_label($data->{$this->name_alias}));
  }

  /**
   * Override the behavior of title().
   */
  public function title() {
    return check_plain($this->pattern_type_label($this->argument));
  }

  /**
   * Returns the human readable label of the pattern type.
   *
   * @param string $type
   *   The pattern type machine name.
   *
   * @return string
   *   The un-sanitized human readable label.
   */
  public function pattern_type_label($type) {
    $output = patternbuilder_pattern_types_labels($type);
    if (empty($output)) {
      $output = t('Unknown pattern type');
    }
    return $output;
  }

}
