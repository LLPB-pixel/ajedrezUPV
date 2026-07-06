import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split

class Connect4Model:

    def __init__(
        self,
        width=7,
        height=6,
        layers=(64, 64, 32),
        validation=0.2,
        test=0.1,
        batch_size=10,
        epochs=30,
    ):

        self.width = width
        self.height = height

        self.validation = validation
        self.test = test

        self.batch_size = batch_size
        self.epochs = epochs

        self.model = self._create_model(layers)

    def _create_model(self, layers):

        model = tf.keras.Sequential()

        model.add(
            tf.keras.Input(shape=(self.width * self.height,))
        )

        for neurons in layers:
            model.add(
                tf.keras.layers.Dense(
                    neurons,
                    activation="tanh"
                )
            )

        model.add(
            tf.keras.layers.Dense(
                1,
                activation="linear"
            )
        )

        model.compile(
            optimizer="adam",
            loss="mse",
            metrics=["mae"]
        )

        return model

    def train(self, x, y):

        x_train, x_test, y_train, y_test = train_test_split(
            x,
            y,
            test_size=self.test
        )

        callback = tf.keras.callbacks.EarlyStopping(
            monitor="val_loss",
            patience=5,
            restore_best_weights=True
        )

        self.model.fit(
            x_train,
            y_train,
            validation_split=self.validation,
            epochs=self.epochs,
            batch_size=self.batch_size,
            callbacks=[callback]
        )

        return self.model.evaluate(
            x_test,
            y_test,
            verbose=0
        )

    def predict(self, board):

        board = np.asarray(board).reshape(
            1,
            self.width * self.height
        )

        return self.model.predict(board, verbose=0)[0][0]

    def save(self, path):

        self.model.save(path)

    def load(self, path):

        self.model = tf.keras.models.load_model(path)