'use strict';

// User passed --eval-lis-task` arguments to Node without `-i` or
// `--interactive`.

const {
  prepareMainThreadExecution
} = require('internal/bootstrap/pre_execution');
const { evalScript } = require('internal/process/execution');
const { addBuiltinLibsToObject } = require('internal/modules/cjs/helpers');
const db = require('db');

const { getOptionValue } = require('internal/options');

prepareMainThreadExecution();
addBuiltinLibsToObject(global);
markBootstrapComplete();

const taskId = getOptionValue('--eval-lis-task');
const inputArgsJsonString = getOptionValue('--input-args');

const source = db.executeScalar(
  'select cod from QT_TASKS where num_rec=? and typ=1', taskId);
if (!source) {
  throw new Error(`Script with id: ${taskId} is empty`);
}

if (inputArgsJsonString) {
  const inputArgs = JSON.parse(inputArgsJsonString);
  global.inputArgs = inputArgs;
}

evalScript('[eval]',
           source,
           getOptionValue('--inspect-brk'),
           true);
