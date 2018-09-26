const express = require('express');
const simpleAuth = require('./simpleAuth');
const uuid = require('uuid/v1');
const util = require('util');
const log = require('./message_log');

let userRouter = express.Router();

// Routes
userRouter.post('/SchoolUnitGroup', (req, res) => {
    console.log("POST create SchoolUnitGroup" + " " + new Date());
    let data = util.inspect(req.body, {showHidden: false, depth: null});
    log("out/SchoolUnitGroup.log", util.inspect(req.body, {showHidden: false, depth: null}));
    // console.log(util.inspect(req.body, {showHidden: false, depth: null}));
    let resp = {"id": uuid()};
    res.status(201).json(resp);
});
userRouter.put('/SchoolUnitGroup/:id', (req, res) => {
    console.log("PUT update a user: ", req.params['id'] + " " + new Date());
    console.log(req.body);
    res.status(200).send();
});

// get by db id
userRouter.delete('/SchoolUnitGroup/:id', (req, res) => {
    console.log("DELETE one member by id:" + req.params['id'] + " " + new Date());
    res.sendStatus(204);
});

userRouter.get('/SchoolUnitGroup', (req, res) => {
    res.status(200).send("jodu " + new Date());
});

module.exports = userRouter;
