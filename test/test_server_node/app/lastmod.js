module.exports = exports = function lastModifiedPlugin(schema, options) {
    schema.add({lastMod: Date, upd_by: String});

    schema.pre('save', function (next, req) {
        if (!this.isNew) {
            this.lastMod = new Date();
            this.upd_by = req.auth.user;
        }
        next();
    });

    if (options && options.index) {
        schema.path('lastMod').index(options.index);
    }
};




