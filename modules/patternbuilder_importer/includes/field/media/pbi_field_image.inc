<?php

/**
 * @file
 * Class for creating Drupal image fields for pattern properties.
 */

/**
 * Field image importer class.
 */
class pbi_field_image extends pbi_field_media {

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return parent::dependencies_met() && module_exists('image');
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_defaults() {
    $defaults = array(
      'image_style' => '',
      'preview_image_style' => '',
    );
    $defaults += parent::settings_defaults();
    return $defaults;
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_form() {
    $form = parent::settings_form();
    $storage_id = static::settings_storage_id();
    $settings = static::settings_values();

    $image_styles = image_style_options(FALSE, PASS_THROUGH);
    $form[$storage_id]['image_style'] = array(
      '#title' => t('Image style'),
      '#type' => 'select',
      '#default_value' => $settings['image_style'],
      '#empty_option' => t('None (original image)'),
      '#options' => $image_styles,
    );

    $form[$storage_id]['preview_image_style'] = array(
      '#title' => t('Preview image style'),
      '#type' => 'select',
      '#default_value' => $settings['preview_image_style'],
      '#empty_option' => t('None (original image)'),
      '#options' => $image_styles,
    );

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public static function field_property_map_suggestions() {
    $map = parent::field_property_map_suggestions();

    // Alt text set by file_entity_file_load().
    $map['alt'] = array('alt');

    // Title text set by file_entity_file_load().
    $map['title'] = array('title');

    return $map;
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return 'image';
  }

  /**
   * {@inheritdoc}
   */
  public static function global_allowed_file_types() {
    return array('image');
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_default_field_formatter($property, array $base, array $instance) {
    $formatter = $this->hook_default_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      $settings = static::settings_values();
      $image_style = !empty($settings['image_style']) ? $settings['image_style'] : '';
      $formatter = array(
        'type' => 'image',
        'label' => 'hidden',
        'settings' => array(
          'image_link' => '',
          'image_style' => $image_style,
        ),
      );
    }

    return $formatter;
  }

  /**
   * {@inheritdoc}
   */
  protected function determine_preview_field_formatter($property, array $base, array $instance) {
    $formatter = $this->hook_preview_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      $settings = static::settings_values();
      $image_style = !empty($settings['preview_image_style']) ? $settings['preview_image_style'] : '';
      $formatter = array(
        'type' => 'image',
        'label' => 'hidden',
        'settings' => array(
          'image_link' => '',
          'image_style' => $image_style,
        ),
      );
    }

    return $formatter;
  }

}
