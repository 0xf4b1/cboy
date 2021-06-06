// SPDX-License-Identifier: GPL-3.0-only

package io.github.tiefensuche.anboy

import android.app.ListActivity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.View
import android.widget.ArrayAdapter
import android.widget.ListView
import androidx.appcompat.widget.Toolbar
import java.io.File


class MainActivity : ListActivity() {

    var path: String = "/sdcard"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        intent.dataString?.let {
            path = it
        }

        val mActionBarToolbar = findViewById<Toolbar>(R.id.toolbar)
        mActionBarToolbar.title = getString(R.string.app_name) + " - " + path

        val values = ArrayList<String>()
        val dir = File(path)
        if (dir.canRead()) {
            dir.listFiles { file -> file.isDirectory || file.name.endsWith(".gb") || file.name.endsWith(".gbc") }.forEach { file -> values.add(file.name) }
        }

        listAdapter = ArrayAdapter(this,
                android.R.layout.simple_list_item_2, android.R.id.text1, values)
    }

    override fun onListItemClick(l: ListView, v: View, position: Int, id: Long) {
        var filename = listAdapter.getItem(position) as String
        filename = if (path.endsWith(File.separator)) path + filename else path + File.separator + filename

        if (File(filename).isDirectory) {
            val intent = Intent(this, MainActivity::class.java)
            intent.data = Uri.parse(filename)
            startActivity(intent)
        } else {
            val intent = Intent(this, android.app.NativeActivity::class.java)
            intent.data = Uri.parse(filename)
            startActivity(intent)
        }
    }

}
