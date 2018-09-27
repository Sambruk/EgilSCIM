const express = require('express');
const appConfig = require('./config');
const uuid = require('uuid/v1');
let userRouter = express.Router();
const util = require('util');
const log = require('./message_log');

const inspect_config = {showHidden: false, depth: null};
// Routes
userRouter.post('/Employment', (req, res) => {
    console.log("POST create employment" + " " + new Date());
    log("out/Employment.log", util.inspect(req.body, appConfig.jsonFormat));
    let resp = {"id": uuid()};
    res.status(201).json(resp);
});
userRouter.put('/Employment/:id', (req, res) => {
    console.log("PUT update an employment: ", req.params['id'] + " " + new Date());
    log("out/Employment.log", util.inspect(req.body, appConfig.jsonFormat));
    console.log(req.body);
    res.status(200).send();
});

// get by db id
userRouter.delete('/Employment/:id', (req, res) => {
    console.log("DELETE one employment by id:" + req.params['id'] + " " + new Date());
    res.sendStatus(204);
});

userRouter.get('/Employment', (req, res) => {
    res.status(200).send("jodu " + new Date());
});

module.exports = userRouter;
