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
package com.mdroid.settings.device;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceViewHolder;
import android.util.AttributeSet;

import java.util.List;

public class MicGainPreference extends ProperSeekBarPreference {

    private static int mMinVal = -10;
    private static int mMaxVal = 20;
    private static int mDefVal = 0;

    private static final String MICROPHONE_GAIN_PATH = "/sys/kernel/sound_control/mic_gain";

    public MicGainPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        mInterval = 1;
        mShowSign = false;
        mUnits = "";
        mContinuousUpdates = false;
        mMinValue = mMinVal;
        mMaxValue = mMaxVal;
        mDefaultValueExists = true;
        mDefaultValue = mDefVal;

        setPersistent(false);
    }

    public static boolean isSupported() {
        return Utils.fileWritable(MICROPHONE_GAIN_PATH);
    }

    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        String storedValue = PreferenceManager.getDefaultSharedPreferences(context).getString(DeviceSettings.KEY_MICROPHONE_GAIN, String.valueOf(mDefVal));
        Utils.writeValue(MICROPHONE_GAIN_PATH, storedValue);
    }

    private void saveValue(String newValue) {
        Utils.writeValue(MICROPHONE_GAIN_PATH, newValue);
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
        editor.putString(DeviceSettings.KEY_MICROPHONE_GAIN, newValue);
        editor.commit();
    }

    @Override
    protected void changeValue(int newValue) {
        saveValue(String.valueOf(newValue));
    }
}

