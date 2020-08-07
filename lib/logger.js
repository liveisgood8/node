'use strict';

const {
  trace,
  debug,
  info,
  warning,
  error
} = internalBinding('logger');

module.exports = {
  trace,
  debug,
  info,
  warning,
  error
};