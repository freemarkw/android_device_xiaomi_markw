/*
* Copyright (C) 2016 The OmniROM Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*/
package com.mdroid.settings.device.sound;

import android.os.Bundle;
import android.support.v14.preference.PreferenceFragment;
import android.support.v7.preference.Preference;

import com.mdroid.settings.device.R;

public class SoundControlSettings extends PreferenceFragment {

    public static final String KEY_HEADPHONE_GAIN = "headphone_gain";
    public static final String KEY_SPEAKER_GAIN = "speaker_gain";
    public static final String KEY_MICROPHONE_GAIN = "mic_gain";

    private HeadphoneGainPreference mHeadphoneGain;
    private SpeakerGainPreference mSpeakerGain;
    private MicGainPreference mMicGain;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.sound_control, rootKey);

        mHeadphoneGain = (HeadphoneGainPreference) findPreference(KEY_HEADPHONE_GAIN);
        if (mHeadphoneGain != null) {
            mHeadphoneGain.setEnabled(HeadphoneGainPreference.isSupported());
        }

        mSpeakerGain = (SpeakerGainPreference) findPreference(KEY_SPEAKER_GAIN);
        if (mSpeakerGain != null) {
            mSpeakerGain.setEnabled(SpeakerGainPreference.isSupported());
        }

        mMicGain = (MicGainPreference) findPreference(KEY_MICROPHONE_GAIN);
        if (mMicGain != null) {
            mMicGain.setEnabled(MicGainPreference.isSupported());
        }
    }
}
