const express = require('express');
const simpleAuth = require('./simpleAuth');
const uuid = require('uuid/v1');
let userRouter = express.Router();
const util = require('util');
const log = require('./message_log');

// Routes
userRouter.post('/StudentGroup', (req, res) => {
    console.log("POST create StudentGroup" + " " + new Date());
    let data = util.inspect(req.body, {showHidden: false, depth: null});
    log("out/StudentGroup.log", util.inspect(req.body, {showHidden: false, depth: null}));
    // console.log(util.inspect(req.body, {showHidden: false, depth: null}));
    let resp = {"id": uuid()};
    res.status(201).json(resp);
});
userRouter.put('/StudentGroup/:id', (req, res) => {
    console.log("PUT update an StudentGroup: ", req.params['id'] + " " + new Date());
    console.log(req.body);
    res.status(200).send();
});

// get by db id
userRouter.delete('/StudentGroup/:id', (req, res) => {
    console.log("DELETE one StudentGroup by id:" + req.params['id'] + " " + new Date());
    res.sendStatus(204);
});

userRouter.get('/StudentGroup', (req, res) => {
    res.status(200).send("jodu " + new Date());
});

module.exports = userRouter;
