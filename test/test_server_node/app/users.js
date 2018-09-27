const express = require('express');
const simpleAuth = require('./simpleAuth');
const appConfig = require('./config');
const uuid = require('uuid/v1');
const util = require('util');
const log = require('./message_log');

let userRouter = express.Router();

// Routes
userRouter.post('/Users', (req, res) => {
    console.log("POST create user" + " " + new Date());
    log("out/User.log", util.inspect(req.body, appConfig.jsonFormat));
    let resp = {"id": uuid()};
    res.status(201).json(resp);
});
userRouter.put('/Users/:id', (req, res) => {
    console.log("PUT update a user: ", req.params['id'] + " " + new Date());
    log("out/User.log", util.inspect(req.body, appConfig.jsonFormat));
    res.status(200).send();
});

// get by db id
userRouter.delete('/Users/:id', (req, res) => {
    console.log("DELETE one member by id:" + req.params['id'] + " " + new Date());
    res.sendStatus(204);
});

userRouter.get('/Users', (req, res) => {
    res.status(200).send("jodu " + new Date());
});

module.exports = userRouter;
