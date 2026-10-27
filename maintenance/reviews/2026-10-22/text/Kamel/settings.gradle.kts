pluginManagement {
    repositories {
        maven("https://maven.eazytec-cloud.com/nexus/repository/maven-public/")
        maven("https://maven.eazytec-cloud.com/nexus/content/groups/harmonyos")
        gradlePluginPortal()
        mavenCentral()
        google()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.PREFER_PROJECT)
    repositories {
        maven("https://maven.eazytec-cloud.com/nexus/repository/maven-public/")
        maven("https://maven.eazytec-cloud.com/nexus/content/groups/harmonyos")
        google()
        mavenCentral()
    }
}

rootProject.name = "Kamel"

include(":kamel-core")
include(":example:nativeApp")
