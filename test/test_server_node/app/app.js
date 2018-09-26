
const serverConfig = require('./config');
const express = require('express');

const app = express();

// app.use(require('scim-node'));
app.use(require('helmet')());

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
app.use(require('./users'));
app.use(require('./employment'));
app.use(require('./schoolunit'));
app.use(require('./studentgroup'));
app.use(require('./schoolunitgroup'));
app.use(require('./employment'));
app.use(require('./activity'));

if (serverConfig.https) {
// Listen over https
    serverConfig.secureServer
        .createServer(serverConfig.httpsOptions, app)
        .listen(serverConfig.port, () => {
            console.log("Listening on " + serverConfig.port + " with ssl/tls");
        });
} else {
// Listen over plaintext
    app.listen(serverConfig.port, () => {
        console.log("Listening on " + serverConfig.port + " unencrypted");
    });
}

