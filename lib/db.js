// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

'use strict';

const {
  openConnection,
  closeConnection,
  Query
} = internalBinding('db');


open();

// Bind connection close on process exit
process.on('exit', () => closeConnection());

/**
 * @typedef {Object} ExecuteResult
 * @property {Array<Array>} rows - result rows of query
 * @property {number} rowsAffected - number of affected rows
 * @property {number} insertedId - id of inserted row if query action is insert
 */

/**
 * Execute query and call `callback` for each row
 * @param {*} sql - sql query string 
 * @param {Array<any> | function} parameters - parameters for binding to query or callback
 * @param {*} callback - callback function for each row (row) => {...}
 */
function each(sql, parametersOrCallback, callback) {
  let result = this.execute(sql, typeof parametersOrCallback !== 'function' ? parametersOrCallback : null);
  if (!result.rows) {
    logger.warning(`db::each - empty result for query: ${sql}`);
    return;
  }

  result.rows.forEach(row => {
    if (typeof parametersOrCallback === 'function') {
      parametersOrCallback(row);
    } else {
      callback(row);
    }
  });
}

/**
 * Prepare and execute query to database
 * @param {string} sql - sql query string 
 * @param {Array<any>} parameters - parameters for binding to query 
 * @returns {ExecuteResult} - result of executing
 */
function execute(sql, parameters) {
  let query = this.prepare(sql);

  if (parameters !== null && parameters !== undefined) {
    let isParamsPassedAsArray = Array.isArray(parameters);
    if ((typeof parameters === 'object' && !(parameters instanceof Date)) 
      && !isParamsPassedAsArray) {
      throw new Error('Query parameters could not be an object');
    }

    if (!isParamsPassedAsArray) {
      parameters = [parameters];
    }

    parameters.forEach(p => {
      query.addParameter(p);
    });
  }

  if (!query.execute()) {
    throw new Error(query.lastError());
  }

  if (!query.first()) {
    return {
      rows: null,
      rowsAffected: query.numRowsAffected(),
      insertedId: query.lastInsertId()
    };
  }

  let _rows = [];
  do {
    let _row = [];

    const fieldsCount = query.fieldsCount();
    for (let i = 0; i < fieldsCount; i++) {
      _row.push(query.fieldValue(i));
    }

    _rows.push(_row);
  } while (query.next());

  return {
    rows: _rows,
    numRowsAffected: query.numRowsAffected(),
    insertedId: query.lastInsertId()
  };
}

/**
 * Prepare and execute query to database
 * @param {string} sql - sql query string 
 * @param {Array<any>} parameters - parameters for binding to query 
 * @returns {any} - value of first cell 
 */
function executeScalar(sql, parameters) {
  let result = this.execute(sql, parameters);

  if (result.rows 
    && result.rows.length 
    && result.rows[0].length) {
    return result.rows[0][0];
  }

  return null;
}

/**
 * Prepare query 
 * @param {string} sql - sql query string 
 * @returns {Query} - query object
 */
function prepare(sql) {
  let query = new Query();

  if (!query.prepare(sql)) {
    throw new Error(query.lastError())
  }

  return query;
}

function open() {
  try {
    openConnection();
  }
  catch (err) {
    logger.error('Database::open:', err.message);
    throw err;
  }
}

/**
 * Close native database connection
 */
function close() {
    closeConnection();
}

module.exports = {
    each,
    execute,
    executeScalar,
    prepare,
    close
}