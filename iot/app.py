import os
import joblib
import pandas as pd
from flask import Flask, request, jsonify
from flask_cors import CORS
from sklearn.preprocessing import StandardScaler

app = Flask(__name__)
CORS(app)

app.config['MODEL_FOLDER'] = './models'

# Load the trained LDA model
def load_trained_model():
    model = joblib.load(f"{app.config['MODEL_FOLDER']}/best_lda_model.joblib")
    return model

# Load the scaler used for feature scaling
def load_scaler():
    scaler = joblib.load(f"{app.config['MODEL_FOLDER']}/scaler.joblib")
    return scaler

# Route to make predictions
@app.route('/predict', methods=['POST'])
def predict():
    data = request.json

    # Extract data from the JSON payload
    temperature = data.get('temperature')
    humidity = data.get('humidity')
    pressure = data.get('pressure')
    dewPoint = data.get('dewPoint')
    cloud = data.get('cloud')
    windspeed = data.get('windspeed')
    winddirection = data.get('winddirection')
    sunshine = data.get('sunshine')

    # Check if any of the required fields are missing
    if any(v is None for v in [temperature, humidity, pressure, dewPoint, cloud, windspeed, winddirection, sunshine]):
        return jsonify({"error": "Missing one or more input values."}), 400

    # Prepare the features for prediction
    features = [[pressure, temperature, dewPoint, humidity, cloud, sunshine, winddirection, windspeed]]

    # Load the scaler and scale the features
    scaler = load_scaler()
    features_scaled = scaler.transform(features)

    # Load the model and make predictions
    model = load_trained_model()
    prediction = model.predict(features_scaled)

    # Return the prediction as a JSON response
    return jsonify({"Predicted_Rainfall": int(prediction[0])})  # Assuming the prediction is binary (0 or 1)

if __name__ == '__main__':
    # Ensure required directories exist
    os.makedirs(app.config['MODEL_FOLDER'], exist_ok=True)

    # Load the scaler for feature scaling
    scaler = StandardScaler()
    joblib.dump(scaler, f"{app.config['MODEL_FOLDER']}/scaler.joblib")  # Adjust if you need to save a trained scaler

    app.run(debug=True)
