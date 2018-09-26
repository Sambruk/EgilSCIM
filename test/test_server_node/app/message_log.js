const fs = require('fs');

module.exports = (path, data) => {
    fs.open(path, 'a', (err, fd) => {
        if (err) {
            throw 'error opening file: ' + err;
        }

        fs.appendFile(fd, data + "\n---\n", (err) => {
            fs.close(fd, (err) => {
                if (err) throw err;
            });
            if (err) throw err;
        });
    });
};