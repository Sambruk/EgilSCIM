
const appConfig = require('./config');
const express = require('express');

const app = express();

// app.use(require('scim-node'));
app.use(require('helmet')());
app.use(require('morgan')('dev'));

app.use((req, res, next) => {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Methods", "GET,HEAD,OPTIONS,POST,PUT, DELETE");
    res.header("Access-Control-Allow-Headers", "Origin," +
        " X-Requested-With, Content-Type, Accept, Authorization");
    next();
});
app.get('/', function (req, res) {
    res.send('GET request to the homepage')
});

app.use(express.json());
app.use(express.urlencoded({extended: true}));

app.use('/users', require('./generic_logging_route'));
app.use('/schoolunit', require('./generic_logging_route'));
app.use('/studentgroup', require('./generic_logging_route'));
app.use('/schoolunitgroup', require('./generic_logging_route'));
app.use('/employment', require('./generic_logging_route'));
app.use('/activity', require('./generic_logging_route'));

if (appConfig.https) {
// Listen over https
    appConfig.secureServer
        .createServer(appConfig.httpsOptions, app)
        .listen(appConfig.port, () => {
            console.log("Listening on " + appConfig.port + " with ssl/tls");
        });
} else {
// Listen over plaintext
    app.listen(appConfig.port, () => {
        console.log("Listening on " + appConfig.port + " unencrypted");
    });
}

