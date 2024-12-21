import os
import joblib
import pandas as pd
from flask import Flask, request, jsonify
from flask_cors import CORS
from sklearn.preprocessing import StandardScaler
import numpy as np

app = Flask(__name__)
#CORS(app)

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
    print(f"Received data: {data}")

    # Extract data from the JSON payload
    '''temperature = data.get('temperature')
    humidity = data.get('humidity')
    pressure = data.get('pressure')
    dewPoint = data.get('dewPoint')
    cloud = data.get('cloud')
    windspeed = data.get('windspeed')
    winddirection = data.get('winddirection')
    sunshine = data.get('sunshine')'''
    try:
        temperature = data['temperature']
        humidity = data['humidity']
        pressure = data['pressure']
        dew_point = data['dewPoint']
        cloud = data['cloud']
        windspeed = data['windspeed']
        winddirection = data['winddirection']
        sunshine = data['sunshine']
    except KeyError:
        return jsonify({"error": "Invalid input format"}), 400

    # Check if any of the required fields are missing
    if any(v is None for v in [temperature, humidity, pressure, dew_point, cloud, windspeed, winddirection, sunshine]):
        return jsonify({"error": "Missing one or more input values."}), 400

    # Prepare the features for prediction
    #features = [[pressure, temperature, dewPoint, humidity, cloud, sunshine, winddirection, windspeed]]
    # Create a feature vector for the model
    #features = np.array([[pressure, temperature, dew_point, humidity, cloud, sunshine, winddirection, windspeed]])
    features = pd.DataFrame([[pressure, temperature, dew_point, humidity, cloud, sunshine, winddirection, windspeed]], 
                        columns=['pressure', 'temparature', 'dewpoint', 'humidity', 'cloud', 'sunshine', 'winddirection', 'windspeed'])



    # Load the scaler and scale the features
    scaler = load_scaler()
    features_scaled = scaler.transform(features)

    # Load the model and make predictions
    model = load_trained_model()
    prediction = model.predict(features_scaled)
    prediction_proba = model.predict_proba(features)[:, 1]
    

    # Return the prediction as a JSON response
    # Return the prediction and probability
    result = {
        "prediction": int(prediction[0]),  # 1 for rainfall, 0 for no rainfall
        "probability_of_rainfall": float(prediction_proba[0])
    }
    print(result)
    return jsonify(result)

if __name__ == '__main__':
    # Ensure required directories exist
    os.makedirs(app.config['MODEL_FOLDER'], exist_ok=True)

    # Load the scaler for feature scaling
    #scaler = StandardScaler()
    #joblib.dump(scaler, f"{app.config['MODEL_FOLDER']}/scaler.joblib")  # Adjust if you need to save a trained scaler

    #app.run(debug=True)
    app.run(host="0.0.0.0", port=5000,debug=True)
