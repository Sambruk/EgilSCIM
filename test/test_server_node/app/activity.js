const express = require('express');
const appConfig = require('./config');
const uuid = require('uuid/v1');
const util = require('util');
const log = require('./message_log');

let userRouter = express.Router();

// Routes
userRouter.post('/Activity', (req, res) => {
    console.log("POST create activity" + " " + new Date());
    log("out/Activity.log", util.inspect(req.body, appConfig.jsonFormat));
    let resp = {"id": uuid()};
    res.status(201).json(resp);
});
// userRouter.put('/Activity/:id', (req, res) => {
userRouter.put('/Activity', (req, res) => {
    console.log("PUT update a activity: " + new Date());
    log("out/Activity.log", util.inspect(req.body, appConfig.jsonFormat));
    console.log(req.body);
    res.status(200).send();
});

// get by db id
userRouter.delete('/Activity/:id', (req, res) => {
    console.log("DELETE one member by id:" + req.params['id'] + " " + new Date());
    res.sendStatus(204);
});

userRouter.get('/Activity', (req, res) => {
    res.status(200).send("jodu " + new Date());
});

module.exports = userRouter;
