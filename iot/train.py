import pandas as pd
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis
from sklearn.preprocessing import StandardScaler
from imblearn.over_sampling import RandomOverSampler
from joblib import dump

# Step 1: Load the dataset
df = pd.read_csv('Rainfall.csv')

# Display the first few rows of the DataFrame
print(df.head())

# Step 2: Preprocess the data
# Rename columns to remove leading/trailing whitespace
df.rename(str.strip, axis='columns', inplace=True)

# Fill missing values with column means
for col in df.columns:
    if df[col].isnull().sum() > 0:
        val = df[col].mean()
        df[col] = df[col].fillna(val)

# Replace categorical values with numerical ones
df.replace({'yes': 1, 'no': 0}, inplace=True)

# Drop unnecessary columns
df.drop(['maxtemp', 'mintemp', 'day'], axis=1, inplace=True)

# Step 3: Prepare features and target variable
features = df.drop(['rainfall'], axis=1)
target = df['rainfall']

# Step 4: Random Oversampling
ros = RandomOverSampler(sampling_strategy='minority', random_state=22)
X_resampled, Y_resampled = ros.fit_resample(features, target)

# Step 5: Feature Scaling
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X_resampled)

# Step 6: Train the LDA Model
best_params = {'priors': None, 'shrinkage': None, 'solver': 'svd'}
lda_model = LinearDiscriminantAnalysis(**best_params)
lda_model.fit(X_scaled, Y_resampled)

# Step 7: Save the trained model
dump(scaler, 'scaler.joblib')
dump(lda_model, 'best_lda_model.joblib')
print("Model saved as 'best_lda_model.joblib'")
