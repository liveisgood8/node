'use strict';

const {
  warning: logWarning,
  error: logError
} = internalBinding('logger');

const {
  type,
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
  const result = this.execute(sql, typeof parametersOrCallback !== 'function' ? parametersOrCallback : null);
  if (!result.rows) {
    logWarning(`db::each - empty result for query: ${sql}`);
    return;
  }

  result.rows.forEach((row) => {
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
  const query = this.prepare(sql);

  if (parameters !== null && parameters !== undefined) {
    const isParamsPassedAsArray = Array.isArray(parameters);
    if ((typeof parameters === 'object' && !(parameters instanceof Date)) &&
      !isParamsPassedAsArray) {
      throw new Error('Query parameters could not be an object');
    }

    if (!isParamsPassedAsArray) {
      parameters = [parameters];
    }

    parameters.forEach((p) => {
      if (p && typeof p === 'object' && p.value instanceof Date) {
        // Additional properties for param
        if (p.isOnlyDate) {
          query.addDateParameter(p.value.getDate(), p.value.getMonth() + 1, p.value.getFullYear());
        } else if (p.isOnlyTime) {
          query.addTimeParameter(p.value.getHours(), p.value.getMinutes(), p.value.getSeconds());
        }
      } else {
        query.addParameter(p);
      }
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

  const _rows = [];
  do {
    const _row = [];

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
  const result = this.execute(sql, parameters);

  if (result.rows && 
    result.rows.length && 
    result.rows[0].length) {
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
  const query = new Query();

  if (!query.prepare(sql)) {
    throw new Error(query.lastError())
  }

  return query;
}

/**
 * @typedef {Object} FieldInfo
 * @property {string} name Field name
 * @property {'boolean' | 'integer' | 'double' | 'string' | 'date'} type Field type
 * @property {boolean} isGenerated Is field auto-generated
 * @property {number} length Field length
 */

/**
 * Return query fields info
 * @param {any} sql Query text
 * @return {FieldInfo[] | null} Array of fields info objects or null if prepare or execute failed
 */
function fieldsInfo(sql) {
  const query = prepare(sql);
  if (query.execute()) {
    return query.fieldsInfo();
  }
  return null;
}

function open() {
  try {
    openConnection();
  } catch (err) {
    logError('Database::open:', err.message);
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
  fieldsInfo,
  close,
  type,
}
