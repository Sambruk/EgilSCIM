const fs = require('fs');

const useHttps = true;

module.exports = {
    local: false,
    https: useHttps,
    port: 9876,
    secureServer: require('https'),
    httpsOptions: {
        key: fs.readFileSync("/etc/certificate/serverkey.pem"),
        cert: fs.readFileSync("/etc/certificate/servercert.pem")
    },
    jsonFormat: {
        compact: false,
        showHidden: false,
        depth: null
    }
};

