const fs = require('fs');

const useHttps = true;

module.exports = {
    local: false,
    https: useHttps,
    port: 9876,
    secureServer: require('https'),
    httpsOptions: {
        key: fs.readFileSync("/etc/SimpleSCIM/test-key.pem"),
        cert: fs.readFileSync("/etc/SimpleSCIM/test-cert.pem")
    },
    jsonFormat: {
        compact: false,
        showHidden: false,
        depth: null
    }
};

