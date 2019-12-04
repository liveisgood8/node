const {
    kIcons,
    kButtons,
    kResults,
    showMessageBox
} = internalBinding('message');

function show(text, title, flags) {
    if (!title) {
        title = 'Внимание';
    }

    if (!flags) {
        flags = 0;
    }

    return showMessageBox(text, title, flags);
}

module.exports = {
    show,
    icons: Object.freeze(kIcons),
    buttons: Object.freeze(kButtons),
    results: Object.freeze(kResults)
};