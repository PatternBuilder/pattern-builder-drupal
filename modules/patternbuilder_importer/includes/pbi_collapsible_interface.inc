<?php

/**
 * @file
 * Interface for collapsible fields.
 */

/**
 * Base collapsible interface.
 */
interface pbi_collapsible_interface {

  /**
   * Detect if the property is collapsible on forms.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is collapsible.
   */
  public function is_collapsible($property);

  /**
   * Determine if the property is collapsed by default on forms.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the property is collpsed.
   */
  public function collapsible_default_collapsed($property);

}
