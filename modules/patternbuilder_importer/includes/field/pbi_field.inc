<?php

/**
 * @file
 * Class for creating Drupal fields for pattern properties.
 */

/**
 * Base field importer class.
 */
class pbi_field implements pbi_field_interface {

  const NAME_PREFIX = 'field';
  const NAME_MAX_LEN = 32;

  protected $id;
  protected $bundleName;
  protected $entityType = 'paragraphs_item';
  protected $createFieldBase = TRUE;

  /**
   * Class Construct.
   *
   * @param int|string $id
   *   ID for the pattern.
   * @param string $bundle_name
   *   The machine name of the bundle.
   * @param string|null $entity_type
   *   The entity type. If not provided, then the default entity type is used.
   */
  public function __construct($id, $bundle_name, $entity_type = NULL) {
    $this->id = $id;
    $this->bundleName = $bundle_name;

    if (isset($entity_type)) {
      $this->entityType = $entity_type;
    }
  }

  /**
   * {@inheritdoc}
   */
  public static function dependencies_met() {
    return TRUE;
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_form() {
    $form = array();
    $classname = get_called_class();
    $my_classname = get_class();
    if ($classname === $my_classname) {
      $settings = static::settings_values();
      $storage_id = static::settings_storage_id();

      $form[$storage_id] = array(
        '#type' => 'fieldset',
        '#title' => $classname,
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#tree' => TRUE,
      );

      $form[$storage_id]['text_preview_trim_length'] = array(
        '#title' => t('Text field preview trimmed limit'),
        '#type' => 'textfield',
        '#field_suffix' => t('characters'),
        '#size' => 10,
        '#default_value' => $settings['text_preview_trim_length'],
        '#element_validate' => array('element_validate_integer_positive'),
        '#description' => t('If the summary is not set, the trimmed text field will be shorter than this character limit.'),
      );
    }
    return $form;
  }

  /**
   * Wrapper of field_ui_widget_type_options().
   *
   * Provides widget options without requiring the field_ui module.
   *
   * @param string $field_type
   *   The field type.
   *
   * @return array
   *   An array of widget options keyed by type.
   */
  public static function field_widget_type_options($field_type) {
    $options = array();
    $field_type_info = field_info_field_types($field_type);
    if (!empty($field_type_info)) {
      foreach (field_info_widget_types() as $name => $widget_type) {
        if (in_array($field_type, $widget_type['field types'], TRUE)) {
          $options[$name] = $widget_type['label'];
        }
      }
    }

    return $options;
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_values() {
    $defaults = static::settings_defaults();
    if (!$defaults) {
      $defaults = array();
    }

    $stored = static::settings_stored();
    return $stored ? $stored + $defaults : $defaults;
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_defaults() {
    $defaults = array();

    if (get_called_class() === get_class()) {
      $defaults['text_preview_trim_length'] = 200;
    }
    return $defaults;
  }

  /**
   * Retrieve the settings storage id.
   *
   * @return string
   *   The storage id.
   */
  public static function settings_storage_id() {
    return 'pbi_handler_' . get_called_class();
  }

  /**
   * {@inheritdoc}
   */
  public static function settings_stored() {
    $id = static::settings_storage_id();
    $stored = $id ? variable_get($id, array()) : array();

    $classname = get_called_class();
    $parent_classname = get_parent_class($classname);
    if (method_exists($parent_classname, 'settings_stored')) {
      $stored += $parent_classname::settings_stored();
    }

    return $stored;
  }

  /**
   * Extracts settings with a given prefix.
   *
   * @param string $prefix
   *   The prefix to find.
   *
   * @return array
   *   An array of values keyed by the field instance settings name.
   */
  public static function handler_extract_settings($prefix) {
    $prefix_len = drupal_strlen($prefix);
    $extracted_settings = array();
    $settings = static::settings_values();
    foreach ($settings as $setting_name => $setting_value) {
      if (strpos($setting_name, $prefix) === 0) {
        $extracted_setting_name = substr($setting_name, $prefix_len);
        $extracted_settings[$extracted_setting_name] = $setting_value;
      }
    }

    return $extracted_settings;
  }

  /**
   * Extracts field settings from the handler settings.
   *
   * @return array
   *   An array of values keyed by the field settings name.
   */
  public static function handler_field_settings() {
    return static::handler_extract_settings('field__');
  }

  /**
   * Extracts instance settings from the handler settings.
   *
   * @return array
   *   An array of values keyed by the field instance settings name.
   */
  public static function handler_instance_settings() {
    return static::handler_extract_settings('instance__');
  }

  /**
   * Extracts widget settings from the handler settings.
   *
   * @return array
   *   An array of values keyed by the field widget settings name.
   */
  public static function handler_widget_settings() {
    return static::handler_extract_settings('widget__');
  }

  /**
   * {@inheritdoc}
   */
  public function save($name, $property, array $required_properties, &$weight) {
    $weight++;
    if ($field = $this->preprocess_field($name, $property, $required_properties, $weight)) {
      $field_instance = $this->add_to_bundle($field);
      return $field_instance ? array($field_instance) : array();
    }
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_type() {
    return NULL;
  }

  /**
   * {@inheritdoc}
   */
  public static function global_field_value_key() {
    return 'value';
  }

  /**
   * Calculate the Drupal field type.
   *
   * @param object $property
   *   The full property object.
   * @param string $property_type
   *   The property type resolved.
   * @param bool $is_enum
   *   Does the field have an enumerated value.
   *
   * @return mixed
   *   The Drupal field type.
   */
  protected function determine_field_type($property, $property_type = NULL, $is_enum = FALSE) {
    // Check global field type.
    $field_type = static::global_field_type();
    if ($field_type) {
      return $field_type;
    }

    $field_type = NULL;

    // Determine property type.
    $property_type = isset($property_type) ? $property_type : (isset($property->type) ? $property->type : NULL);
    if (!isset($property_type)) {
      return $field_type;
    }

    switch ($property_type) {
      case 'field_collection':
        $field_type = 'field_collection';
        break;

      case 'array':
        $field_type = 'text';
        $items = pbi_utility::resolve_property_items($property);
        if (!empty($items->items) && isset($items->type) && $items->type) {
          $top_item = reset($items->items);
          $field_type = $this->determine_field_type($top_item, $items->type, $is_enum);
        }
        elseif ($is_enum) {
          $field_type = 'list_text';
        }
        elseif (pbi_utility::property_is_textarea($property)) {
          $field_type = 'text_long';
        }
        break;

      case 'boolean':
        $field_type = 'list_boolean';
        break;

      case 'integer':
        $field_type = $is_enum ? 'list_integer' : 'number_integer';
        break;

      case 'number':
        $field_type = $is_enum ? 'list_float' : 'number_float';
        break;

      case 'object':
        $field_type = 'field_group';
        break;

      case 'string':
      case 'text':
        $field_type = 'text';
        if ($is_enum) {
          $field_type = 'list_text';
        }
        elseif (pbi_utility::property_is_textarea($property)) {
          $field_type = 'text_long';
        }
        break;

      case 'textarea':
        $field_type = 'text_long';
        break;

      default:
        $field_type = NULL;
    }
    return $field_type;
  }

  /**
   * Determine the system default field instance settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of field instance settings.
   */
  protected function system_default_field_instance_settings($property, array $base, array $instance) {
    $default = array();

    if (!empty($base['type']) && ($type_info = field_info_field_types($base['type']))) {
      if (!empty($type_info['instance_settings'])) {
        $default = $type_info['instance_settings'];
      }
    }

    return $default;
  }

  /**
   * Determine field instance settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of field instance settings.
   */
  protected function determine_field_instance_settings($property, array $base, array $instance) {
    $settings = array(
      'patternbuilder' => array(
        'property_name' => $property->_pb_property_name,
        'parent_property_names' => '',
        'readonly' => isset($property->readonly) && pbi_utility::is_boolean_set($property->readonly),
        'hidden' => isset($property->options->hidden) && pbi_utility::is_boolean_set($property->options->hidden),
        'grid_format' => pbi_utility::property_is_grid_format($property),
        'grid_columns' => pbi_utility::property_grid_columns($property),
      ),
    );

    // Enable input filters for html fields.
    if (isset($base['type']) && $base['type'] == 'text_long' && pbi_utility::property_allows_html($property)) {
      $settings['text_processing'] = 1;
    }

    // Merge with system default settings.
    $settings += $this->system_default_field_instance_settings($property, $base, $instance);

    // Set handler instance settings to override defaults.
    $handler_settings = static::handler_instance_settings();
    if ($handler_settings) {
      $settings = $handler_settings + $settings;
    }

    // Allow others to alter.
    $context = array(
      'base' => $base,
      'instance' => $instance,
      'property' => $property,
      'handler' => $this,
    );
    drupal_alter('patternbuilder_importer_field_instance_settings', $settings, $context);

    return $settings;
  }

  /**
   * Determine the system default field formatter settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   A display array for the default field formatter.
   */
  protected function system_default_field_formatter($property, array $base, array $instance) {
    if (empty($base['type'])) {
      return array();
    }

    $type = $base['type'];
    $type_info = field_info_field_types($type);

    $allowed = array();
    foreach (field_info_formatter_types() as $formatter_name => $formatter) {
      if (in_array($type, $formatter['field types'], TRUE)) {
        $allowed[$formatter_name] = array(
          'type' => $formatter_name,
          'settings' => !empty($formatter['settings']) ? $formatter['settings'] : array(),
        );
      }
    }

    $default = array();
    if (!empty($type_info['default_formatter']) && isset($allowed[$type_info['default_formatter']])) {
      $default = $allowed[$type_info['default_formatter']];
    }
    elseif ($allowed) {
      $default = reset($allowed);
    }

    return $default;
  }

  /**
   * Invoke the Drupal hook to determine the default field formatter settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   A display array for the default field formatter.
   */
  protected function hook_default_field_formatter($property, array $base, array $instance) {
    $formatter = patternbuilder_importer_get_property_default_formatter($property, $base, $instance);
    return $formatter ? $formatter : array();
  }

  /**
   * Determine default field formatter settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   A display array for the default field formatter.
   */
  protected function determine_default_field_formatter($property, array $base, array $instance) {
    $formatter = $this->hook_default_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      // Explicitly set the default formatter to allow field type changes
      // that are allowed due to no data stored or due to type conversions.
      $formatter = $this->system_default_field_formatter($property, $base, $instance);
    }

    return $formatter;
  }

  /**
   * Invoke the Drupal hook to determine the preview field formatter settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   A display array for the default field formatter.
   */
  protected function hook_preview_field_formatter($property, array $base, array $instance) {
    $formatter = patternbuilder_importer_get_property_preview_formatter($property, $base, $instance);
    return $formatter ? $formatter : array();
  }

  /**
   * Determine preview field formatter settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   A display array for the preview field formatter.
   */
  protected function determine_preview_field_formatter($property, array $base, array $instance) {
    $formatter = $this->hook_preview_field_formatter($property, $base, $instance);
    if (empty($formatter)) {
      // Native defaults for some types.
      if (!empty($base['type'])) {
        // Text fields.
        $text_field_types = array('text', 'text_long', 'text_with_summary');
        if (in_array($base['type'], $text_field_types, TRUE)) {
          $settings = static::settings_values();
          // Provide some sane defaults.
          if (module_exists('smart_trim')) {
            $formatter = array(
              'type' => 'smart_trim_format',
              'label' => 'hidden',
              'module' => 'smart_trim',
              'weight' => 1,
              'settings' => array(
                'more_link' => 0,
                'more_text' => 'Read more',
                'summary_handler' => 'trim',
                'trim_type' => 'chars',
                'trim_length' => $settings['text_preview_trim_length'],
                'trim_link' => 0,
                'trim_preserve_tags' => '',
                'trim_suffix' => '',
                'trim_options' => array(
                  'smart_boundaries' => 'smart_boundaries',
                  'text' => 'text',
                ),
              ),
            );
          }
          else {
            $formatter = array(
              'type' => $base['type'] === 'text_with_summary' ? 'text_summary_or_trimmed' : 'text_trimmed',
              'label' => 'hidden',
              'weight' => 1,
              'settings' => array(
                'trim_length' => $settings['text_preview_trim_length'],
              ),
            );
          }
        }
      }

      // Explicitly set the preview formatter to allow field type changes
      // that are allowed due to no data stored or due to type conversions.
      if (empty($formatter)) {
        $formatter = $this->system_default_field_formatter($property, $base, $instance);
      }
    }

    return $formatter;
  }

  /**
   * Returns a stub field item array for the field type.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   *
   * @return array
   *   The field item array with values of NULL.
   */
  protected function field_item_stub($property, array $base) {
    $item = array();
    $field_type = NULL;
    if (!empty($base['type'])) {
      $field_type = $base['type'];
    }
    elseif ($global_field_type = static::global_field_type()) {
      $field_type = $global_field_type;
    }

    if (isset($field_type)) {
      $type_info = field_info_field_types($field_type);
      if (!empty($type_info['module'])) {
        module_load_install($type_info['module']);

        // Merge in default type settings to so modules can use in
        // hook_field_schema().
        if (empty($base['settings'])) {
          $base['settings'] = array();
        }
        $base['settings'] += field_info_field_settings($field_type);

        // Invoke hook_field_schema() to get the columns.
        $schema = (array) module_invoke($type_info['module'], 'field_schema', $base);
        if (!empty($schema['columns'])) {
          foreach ($schema['columns'] as $prop_name => $prop_info) {
            $item[$prop_name] = NULL;
          }
        }
      }
    }

    if (empty($item)) {
      $value_key = static::global_field_value_key();
      if (isset($value_key)) {
        $item[$value_key] = NULL;
      }
    }

    return $item;
  }

  /**
   * Determine the default field item.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   * @param array $allowed_values
   *   An array of allowed values, if any.
   *
   * @return array
   *   The value array to set for the default field value.
   */
  protected function determine_default_field_item($property, array $base, array $instance, array $allowed_values = array()) {
    $item = $this->field_item_stub($property, $base);
    if (!empty($item)) {
      $value = patternbuilder_importer_get_property_default_value($property, $base, $instance, $allowed_values);

      if (!isset($value)) {
        return NULL;
      }
      elseif (is_array($value)) {
        $item = array_intersect_key($value, $item) + $item;
      }
      else {
        reset($item);
        $top_key = key($item);
        $item[$top_key] = $value;
      }

      return array_filter($item) ? $item : NULL;
    }
  }

  /**
   * Determines the field widget to use in Drupal.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of field widget info.
   */
  protected function determine_field_widget($property, array $base, array $instance) {
    $widget = array();

    // Set to handler widget settings to override defaults.
    $handler_settings = static::handler_widget_settings();
    if (!empty($handler_settings)) {
      $widget = $handler_settings;

      // Return if the type is defined.
      if (!empty($widget['type'])) {
        return $widget;
      }
    }

    switch ($base['type']) {
      case 'list_boolean':
        $widget['type'] = 'options_onoff';
        $widget['settings'] = array(
          'display_label' => 1,
        );
        break;

      default:
        $widget = $this->system_default_field_widget($property, $base, $instance);
        break;
    }

    return $widget;
  }

  /**
   * Determine the system field widget settings.
   *
   * @param object $property
   *   The full property object.
   * @param array $base
   *   The built field base array.
   * @param array $instance
   *   The built field instance array.
   *
   * @return array
   *   An array of field widget info.
   */
  protected function system_default_field_widget($property, array $base, array $instance) {
    if (empty($base['type'])) {
      return array();
    }

    $type = $base['type'];
    $type_info = field_info_field_types($type);

    $allowed = array();
    foreach (field_info_widget_types() as $widget_name => $widget) {
      if (in_array($type, $widget['field types'], TRUE)) {
        $allowed[$widget_name] = array(
          'type' => $widget_name,
          'active' => 1,
          'settings' => !empty($widget['settings']) ? $widget['settings'] : array(),
        );
      }
    }

    $default = array();
    if (!empty($type_info['default_widget']) && isset($allowed[$type_info['default_widget']])) {
      $default = $allowed[$type_info['default_widget']];
    }
    elseif ($allowed) {
      $default = reset($allowed);
    }

    return $default;
  }

  /**
   * {@inheritdoc}
   */
  public function create_field_name($name, $property, $id_override = NULL) {
    $id = is_null($id_override) ? (string) $this->id : (string) $id_override;
    $base = isset($property->_pb_base_field_name) ? $property->_pb_base_field_name : static::NAME_PREFIX;
    return pbi_utility::create_unique_name($base, $name, $id, static::NAME_MAX_LEN);
  }

  /**
   * Cleans any internal prefixes from the property name for usage as a label.
   *
   * @param string $name
   *   The property name.
   *
   * @return string
   *   The clean name.
   */
  public static function clean_property_name($name) {
    if (strpos($name, '::') !== FALSE) {
      $parts = explode('::', $name);
      return array_pop($parts);
    }

    return $name;
  }

  /**
   * Pre-process property info into Drupal base and instance field arrays.
   *
   * @param string $name
   *   The name of the property.
   * @param object $property
   *   The property object for a single property.
   * @param array $required_properties
   *   Array of required property names.
   * @param int $weight
   *   Field sorting weight.
   *
   * @return array|bool
   *   Combined array of base and instance, false on failure.
   */
  protected function preprocess_field($name, $property, array $required_properties, $weight) {
    $property_type = isset($property->type) ? $property->type : NULL;

    // Set name on property for easier reference and funtion params.
    $property->_pb_property_name = $name;

    // Initialize base field.
    $field_name = $this->create_field_name($name, $property);
    $base = array(
      'field_name' => $field_name,
      'cardinality' => 1,
      'settings' => array(),
    );

    // Initialize field settings with handler settings.
    $handler_field_settings = static::handler_field_settings();
    if ($handler_field_settings) {
      $base['settings'] = $handler_field_settings;
    }

    // Determine if field is required.
    $is_required = !empty($required_properties) && (in_array($name, $required_properties));

    // Get allowed values.
    $allowed_values = pbi_utility::property_allowed_values($name, $property, $is_required);
    $is_enum = !empty($allowed_values);

    // Set field type.
    $base['type'] = $this->determine_field_type($property, $property_type, $is_enum);
    if (empty($base['type'])) {
      return FALSE;
    }

    // Set boolean allowed values.
    if ($base['type'] == 'list_boolean' && empty($allowed_values)) {
      // Format is array(off value, on value).
      // See list_boolean_allowed_values_callback().
      $allowed_values = array(0, 1);
      $base['settings']['allowed_values'] = $allowed_values;
    }

    if ($allowed_values) {
      // Set allowed values.
      $base['settings']['allowed_values'] = $allowed_values;
    }

    // Set cardinality.
    $max_items = pbi_utility::property_max_items($property);
    if (isset($max_items)) {
      $base['cardinality'] = $max_items;
    }

    // Initialize instance.
    $instance = array(
      'field_name' => $base['field_name'],
      'entity_type' => $this->entityType,
      'bundle' => $this->bundleName,
      'label' => isset($property->title) ? $property->title : static::clean_property_name($name),
      'description' => isset($property->description) ? $property->description : '',
      'widget' => array(
        'active' => 1,
        'weight' => $weight,
      ),
      'required' => $is_required,
      'settings' => array(),
    );

    // Merge instance settings.
    $instance_settings = $this->determine_field_instance_settings($property, $base, $instance);
    if ($instance_settings) {
      $instance['settings'] = drupal_array_merge_deep($instance['settings'], $instance_settings);
    }

    // Merge widget info.
    $widget_info = $this->determine_field_widget($property, $base, $instance);
    if ($widget_info) {
      $instance['widget'] = drupal_array_merge_deep($instance['widget'], $widget_info);
    }

    // Merge default formatter settings.
    $default_formatter = $this->determine_default_field_formatter($property, $base, $instance);
    if ($default_formatter) {
      if (!isset($default_formatter['weight'])) {
        $default_formatter['weight'] = $weight;
      }
      $instance['display']['default'] = $default_formatter;
    }

    // Merge preview formatter settings.
    if (pbi_utility::property_is_preview($property)) {
      $preview_formatter = $this->determine_preview_field_formatter($property, $base, $instance);
      if ($preview_formatter) {
        if (!isset($preview_formatter['weight'])) {
          $preview_formatter['weight'] = $weight;
        }
        $instance['display']['paragraphs_editor_preview'] = $preview_formatter;
      }
    }

    // Set default value.
    $default_item = $this->determine_default_field_item($property, $base, $instance, $allowed_values);
    if (isset($default_item)) {
      $instance['default_value'][0] = $default_item;
    }

    // Build combined return.
    $return = array(
      'base' => $base,
      'instance' => $instance,
    );

    // Allow others to alter.
    $context = array(
      'property' => $property,
      'handler' => $this,
    );
    drupal_alter('patternbuilder_importer_field_import', $return, $context);

    return $return;
  }

  /**
   * Creates Field API fields in the paragraphs entity.
   *
   * Field base: updates are NOT supported.
   * Field instance: updates are supported.
   *
   * @param array $fields_instances
   *   Already processed field names and values.
   * @param bool $create_base_override
   *   Override base field creation. Defaults to class property.
   *
   * @return mixed
   *   The field instance created or updated.
   *
   * @throws \Exception
   *   General error.
   * @throws \FieldException
   *   Field error.
   */
  protected function add_to_bundle(array $fields_instances, $create_base_override = NULL) {
    $base = isset($fields_instances['base']) ? $fields_instances['base'] : array();
    $instance = isset($fields_instances['instance']) ? $fields_instances['instance'] : array();

    $field_name = isset($base['field_name']) ? $base['field_name'] : (isset($instance['field_name']) ? $instance['field_name'] : NULL);
    if (!$field_name) {
      return;
    }

    // Determine creation status.
    $create_base = $this->createFieldBase;
    if (isset($create_base_override)) {
      $create_base = !empty($create_base_override);
    }

    // Field base.
    $base_stored = field_info_field($field_name);
    if ($base_stored) {
      // Existing field base.
      $base_new = $base + $base_stored;
      try {
        field_update_field($base_new);
      }
      catch (FieldException $e) {
        watchdog('patternbuilder_importer', '@field (@field_type): @base_field_error', array(
          '@field' => $field_name,
          '@field_type' => $base_stored['type'] . ($base_new['type'] != $base_stored['type'] ? ' => ' . $base_new['type'] : ''),
          '@base_field_error' => $e->getMessage(),
        ), WATCHDOG_ERROR);
        return;
      }
    }
    elseif ($create_base && $base) {
      // New field base.
      $base_stored = field_create_field($base);
      if (!isset($base_stored['id'])) {
        // Base was not created successfully.
        watchdog('patternbuilder_importer', 'Base field creation failed for @field', array(
          '@field' => $field_name,
        ), WATCHDOG_WARNING);
        return;
      }
    }
    else {
      // Exit: cannot create / update instance without a base.
      watchdog('patternbuilder_importer', 'Missing base field definition for @field', array(
        '@field' => $field_name,
      ), WATCHDOG_WARNING);
      return;
    }

    // Field instance.
    $instance_new = NULL;
    $instance_stored = field_info_instance($instance['entity_type'], $instance['field_name'], $instance['bundle']);
    if (!$instance_stored) {
      // Create field instance.
      $instance_new = field_create_instance($instance);
    }
    else {
      // Update field instance.
      $instance_new = $instance + $instance_stored;
      $instance_equal_count = 0;
      foreach ($instance as $instance_prop_key => $instance_prop_value) {
        if (isset($instance_stored[$instance_prop_key]) && $instance_stored[$instance_prop_key] == $instance_prop_value) {
          $instance_equal_count++;
        }
      }

      if ($instance_equal_count < count($instance)) {
        field_update_instance($instance_new);
      }
    }

    return $instance_new;
  }

}
