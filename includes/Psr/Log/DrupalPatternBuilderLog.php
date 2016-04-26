<?php

/**
 * @file
 * Class to record log messages in Drupal.
 * File is not namespaced in order to work with D7 class loading.
 */

use Psr\Log\LoggerInterface;
use Psr\Log\LogLevel;
use Psr\Log\InvalidArgumentException;

class DrupalPatternBuilderLog implements LoggerInterface {
  protected $onScreen;

  /**
   * Class construct.
   */
  public function __construct() {
    $this->onScreen = variable_get('patternbuilder_devel', FALSE);
  }

  /**
   * System is unusable.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function emergency($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_EMERGENCY);
  }

  /**
   * Action must be taken immediately.
   *
   * Example: Entire website down, database unavailable, etc. This should
   * trigger the SMS alerts and wake you up.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function alert($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_ALERT);
  }

  /**
   * Critical conditions.
   *
   * Example: Application component unavailable, unexpected exception.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function critical($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_CRITICAL);
  }

  /**
   * Runtime errors that do not require immediate action but should typically be logged and monitored.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function error($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_ERROR);
  }

  /**
   * Exceptional occurrences that are not errors.
   *
   * Example: Use of deprecated APIs, poor use of an API, undesirable things
   * that are not necessarily wrong.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function warning($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_WARNING);
  }

  /**
   * Normal but significant events.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function notice($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_NOTICE);
  }

  /**
   * Interesting events.
   *
   * Example: User logs in, SQL logs.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function info($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_INFO);
  }

  /**
   * Detailed debug information.
   *
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function debug($message, array $context = array()) {
    $this->dolog($message, $context, WATCHDOG_DEBUG);
  }

  /**
   * Logs with an arbitrary level.
   *
   * @param mixed $level
   *   Error level.
   * @param string $message
   *   Message with tokens for replacement.
   * @param array $context
   *   Associative array to replace in the message.
   *
   * @return null
   *   Nothing to return.
   */
  public function log($level, $message, array $context = array()) {
    // PSR-3 states that $message should be a string
    $message = (string) $message;

    // map $level to the relevant KLogger method
    switch ($level) {
      case LogLevel::EMERGENCY:
        $this->emergency($message, $context);
        break;

      case LogLevel::ALERT:
        $this->alert($message, $context);
        break;

      case LogLevel::CRITICAL:
        $this->critical($message, $context);
        break;

      case LogLevel::ERROR:
        $this->error($message, $context);
        break;

      case LogLevel::WARNING:
        $this->warning($message, $context);
        break;

      case LogLevel::NOTICE:
        $this->notice($message, $context);
        break;

      case LogLevel::INFO:
        $this->info($message, $context);
        break;

      case LogLevel::DEBUG:
        $this->debug($message);
        break;

      default:
        // PSR-3 states that we must throw a
        // PsrLogInvalidArgumentException if we don't
        // recognize the level
        throw new InvalidArgumentException("Unknown severity level");
    }
  }

  /**
   * Execute logging in Drupal.
   *
   * @param string $message
   *   Message with strings for text
   * @param array() $context
   *   Keyed array of items to replace in message
   * @param int $level
   *   Error level
   *
   * @return null
   *   Nothing to return.
   */
  private function dolog($message, array $context, $level) {
    if ($this->onScreen) {
      drupal_set_message(t($message, $context), 'warning');
    }

    watchdog('patternbuilder', $message, $context, $level);
  }
}
