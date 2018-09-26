const basicAuth = require('express-basic-auth');


const authorize = (username, password, callback) => {
    callback(null, true);
};

module.exports = basicAuth({
    authorizer: authorize,
    authorizeAsync: true
});

