import requests
import pytest
import time

# --- Configuration ---
# Replace with your device's IP if mDNS is not working
BASE_URL = "http://192.168.100.37"
NUMBER_OF_NODES = 19 # Match the value in your firmware


# --- Helper Functions ---
def get_current_profiles():
    """Fetches all current profiles and returns the parsed JSON."""
    response = requests.get(f"{BASE_URL}/getCurrentProfiles")
    response.raise_for_status()  # Will raise an exception for 4xx/5xx errors
    return response.json()

def update_profile(payload):
    """Sends a POST request to the /updateProfile endpoint."""
    return requests.post(f"{BASE_URL}/updateProfile", json=payload)


def test_update_existing_profile():
    """Test modifying the first profile."""
    # Define the changes for profile 0
    active_nodes = [0] * NUMBER_OF_NODES
    active_nodes[5] = 1 # Activate only node 5
    
    payload = {
        "ProfileIndex": 0,
        "ProfileName": "Updated Rainbow",
        "RippleSpeed": 0.75,
        "NumberOfColors": 1,
        "Colors": ["#00FF00"],
        "ActiveNodes": active_nodes
    }

    # Send the update request
    response = update_profile(payload)
    assert response.status_code == 200

    # Verify the changes were applied
    profiles_data = get_current_profiles()
    updated_profile = profiles_data["Profiles"][0]

    assert updated_profile["ProfileName"] == "Updated Rainbow"
    assert updated_profile["RippleSpeed"] == 0.75
    assert updated_profile["NumberOfColors"] == 1
    assert updated_profile["Colors"] == ["#00FF00"]
    assert updated_profile["ActiveNodes"][5] == 1
    assert sum(updated_profile["ActiveNodes"]) == 1

# def test_create_new_profile():
#     """Test adding a second profile."""
#     initial_data = get_current_profiles()
#     initial_profile_count = initial_data["NumberOfActiveProfiles"]

#     # Define the new profile at the next available index
#     payload = {
#         "ProfileIndex": initial_profile_count,
#         "ProfileName": "Second Profile",
#         "Behavior": 2,
#         "NumberOfColors": 1,
#         "Colors": ["#FF00FF"]
#     }

#     response = update_profile(payload)
#     assert response.status_code == 200

#     # Verify the new profile was added
#     new_data = get_current_profiles()
#     assert new_data["NumberOfActiveProfiles"] == initial_profile_count + 1
    
#     new_profile = new_data["Profiles"][initial_profile_count]
#     assert new_profile["ProfileName"] == "Second Profile"
#     assert new_profile["Behavior"] == 2

# def test_delete_profile():
    """Test deleting the second profile that was just created."""
    # Ensure there are 2 profiles before deleting
    initial_data = get_current_profiles()
    assert initial_data["NumberOfActiveProfiles"] == 2

    # Send request to delete profile at index 1
    payload = {"ProfileIndex": 1, "DeleteProfile": True}
    response = update_profile(payload)
    assert response.status_code == 200

    # Verify there is only one profile left
    final_data = get_current_profiles()
    assert final_data["NumberOfActiveProfiles"] == 1
    assert len(final_data["Profiles"]) == 1
    # Check that the remaining profile is the original one
    assert final_data["Profiles"][0]["ProfileName"] == "Updated Rainbow"

    """Test that the API handles invalid JSON gracefully."""
    url = f"{BASE_URL}/updateProfile"
    headers = {"Content-Type": "application/json"}
    malformed_body = '{"ProfileIndex": 0, "ProfileName": "Test", ...' # Incomplete JSON

    response = requests.post(url, headers=headers, data=malformed_body)
    
    assert response.status_code == 400
    assert "Invalid JSON" in response.text