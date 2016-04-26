<?php

/**
 * @file
 * Class to extend the PatternBuilder component in Drupal.
 *
 * File is not namespaced in order to work with D7 class loading.
 */

use PatternBuilder\Property\Component\Component;

/**
 * Class to extend the PatternBuilder component in Drupal.
 */
class DrupalPatternBuilderComponent extends Component {
  // @todo: these should pe protected.
  public $entity;
  public $entityType;

  /**
   * Set a source entity for this component.
   *
   * @param object $entity
   *   The source entity to use for data mappings.
   * @param string $entity_type
   *   The entity type.
   *
   * @return object
   *   Component Object.
   */
  public function source($entity, $entity_type) {
    $this->entity = $entity;
    $this->entityType = $entity_type;

    return $this;
  }

  /**
   * Set a property value from an entity_field.
   *
   * @param string $property_name
   *   The property name to set a value for.
   * @param string $field_name
   *   The properties new value. If you have set a source entity, this can also be a field name.
   * @param bool $append
   *   TRUE if you want to append to the existing value, FALSE if you want to overwrite it.
   *
   * @return object
   *   Component Object.
   */
  public function map($property_name, $field_name, $append = FALSE) {
    // Check if this is a field which can be mapped from the entity.
    if (is_scalar($field_name) && isset($this->entity) && isset($this->entityType)) {
      // Special case title, because Drupal.
      if ($field_name == 'title') {
        return $this->set($property_name, $this->entity->title);
      }

      list($id, $vid, $bundle) = entity_extract_ids($this->entityType, $this->entity);
      $fields = field_info_instances($this->entityType, $bundle);

      if (!empty($fields[$field_name])) {
        $this->setFromEntity($property_name, $field_name, $fields[$field_name]['widget']['type'], $append);
      }
    }

    // Otherwise fall back to the default implementation.
    return $this;
  }

  /**
   * Set a property value from a field.
   *
   * @param string $property_name
   *   The property name.
   * @param string $field_name
   *   The properties new value. If you have set a source entity, this can also be a field name.
   * @param string $field_type
   *   Field type.
   * @param bool $append
   *   TRUE if you want to append to the existing value, FALSE if you want to overwrite it.
   *
   * @return object
   *   Component Object.
   */
  private function setFromEntity($property_name, $field_name, $field_type, $append = FALSE) {
    $items = field_get_items($this->entityType, $this->entity, $field_name);
    if (!empty($items)) {
      // @todo: expand this to add additional default behaviors for field types as we come across them.
      switch ($field_type) {
        case 'text_textarea':
        case 'text_textarea_with_summary':
          foreach ($items as $item) {
            if (!empty($item['format'])) {
              $markup = check_markup($item['value'], $item['format']);
            }
            elseif (!empty($item['safe_value'])) {
              $markup = $item['safe_value'];
            }
            else {
              $markup = $item['value'];
            }

            $this->set($property_name, $markup);
          }
          break;

        case 'text_textfield':
          foreach ($items as $item) {
            $value = empty($item['safe_value']) ? check_plain($item['value']) : $item['safe_value'];
            $this->set($property_name, $value);
          }
          break;

        default:
          $render_array = field_view_field($this->entityType, $this->entity, $field_name);
          $value = render($render_array);
          $this->set($property_name, $value);
          break;
      }
    }

    return $this;
  }

}
