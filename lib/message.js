'use strict';

const {
    kIcons,
    kButtons,
    kResults,
    showMessageBox
} = internalBinding('message');

const {
    codes: {
        ERR_INVALID_ARG_TYPE
    }
} = require('internal/errors');

function show(text, title, flags) {
    if (text === undefined) {
        text = 'undefined';
    } else if (text === null) {
        text = 'null';
    }

    if (typeof text === 'object' && !(text instanceof Date)) {
        text = JSON.stringify(text, null, '\t');
    } else if (typeof text !== 'string') {
        text = text.toString();
    }

    if (title === undefined || title === null) {
        title = 'Внимание';
    } else if (typeof title != 'string') {
        title = title.toString();
    }

    console.log(typeof flags);

    if (flags === undefined || flags === null) {
        flags = 0;
    } else if (typeof flags !== 'number') {
        throw new ERR_INVALID_ARG_TYPE('flags', 'number', flags);
    }

    return showMessageBox(text, title, flags);
}

module.exports = {
    show,
    icons: Object.freeze(kIcons),
    buttons: Object.freeze(kButtons),
    results: Object.freeze(kResults)
};