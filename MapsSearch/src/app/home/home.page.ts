/* eslint-disable @typescript-eslint/member-ordering */
/* eslint-disable @typescript-eslint/naming-convention */
import { Component, ElementRef, ViewChild, NgZone } from '@angular/core';
import { AngularFireAuth } from '@angular/fire/auth';
import { HttpClient } from '@angular/common/http';
import { Geolocation } from '@capacitor/geolocation';
//for search bar
import { MapsAPILoader } from '@agm/core';
import { interval, Subscription } from 'rxjs';

declare let google;

@Component({
  selector: 'app-home',
  templateUrl: 'home.page.html',
  styleUrls: ['home.page.scss'],
})
export class HomePage {

  // Map related
  @ViewChild('map') mapElement: ElementRef;
  mapdata: any;
  markers = [];

  //ambulance co-ordinates
  latitude: any;
  longitude: any;
  source_address: string;

  //destination co-ordinates
  dest_latitude: any;
  dest_longitude: any;
  dest_address: string;

  // Misc
  isTracking = false;
  watch: any;
  user = null;
  flag_source = 0;
  flag_dest = 0;
  initial_flag = 0;

  //for search bar
  @ViewChild('search') searchElementRef: ElementRef;
  private geoCoder;

  //for directions
  directionsService = new google.maps.DirectionsService;
  directionsDisplay = new google.maps.DirectionsRenderer;
  nodes_coords = [
    [19.0710,72.8810],
    [19.0740,72.8840]
  ]
  nodes_address = [];
  subscription: Subscription;
  flag_routing = 0;

  constructor(private afAuth: AngularFireAuth, private httpClient: HttpClient,
    private mapsAPILoader: MapsAPILoader, private ngZone: NgZone) {
    this.anonLogin();

    const source = interval(10000);
    this.subscription = source.subscribe(val => {
      if (this.flag_routing == 1) {
        this.calculateAndDisplayRoute();
      }
    });
  }

  ionViewWillEnter() {
    this.loadMap();
  }

  // Perform an anonymous login and load data
  anonLogin() {
    this.afAuth.signInAnonymously().then(res => {
      this.user = res.user;
    });
  }

  // Initialize a blank map
  loadMap() {
    if (this.initial_flag==0) {
      this.initial_flag = 1;
      const latLng = new google.maps.LatLng(19.0760, 72.8777);

      const mapOptions = {
        center: latLng,
        zoom: 13,
        mapTypeId: google.maps.MapTypeId.ROADMAP
      };

      this.mapdata = new google.maps.Map(this.mapElement.nativeElement, mapOptions);

      for (let j = 0; j < this.nodes_coords.length; j++) {

        const marker = new google.maps.Marker({
          map: this.mapdata,
          animation: google.maps.Animation.DROP,
          position: new google.maps.LatLng(this.nodes_coords[j][0],this.nodes_coords[j][1])
        });
        this.markers.push(marker);

      }
      this.Getnodesaddress();
    }

  }

  // Use Capacitor to track our geolocation
  startTracking() {
    this.isTracking = true;
    this.watch = Geolocation.watchPosition({}, (position, err) => {
      if (position) {
        this.latitude = position.coords.latitude;
        this.longitude = position.coords.longitude;
        this.realtimeupdate(position.coords.latitude,position.coords.longitude);
        this.getAddresssource(position.coords.latitude,position.coords.longitude);

        if (this.flag_source == 0) {
          this.flag_source = 1;
          const latLng = new google.maps.LatLng(position.coords.latitude,position.coords.longitude);

          const mapOptions = {
            center: latLng,
            zoom: 13,
            mapTypeId: google.maps.MapTypeId.ROADMAP
          };

          this.mapdata = new google.maps.Map(this.mapElement.nativeElement, mapOptions);
          this.directionsDisplay.setMap(this.mapdata);
          const marker = new google.maps.Marker({
            map: this.mapdata,
            animation: google.maps.Animation.DROP,
            position: latLng
          });
          this.markers.push(marker);
        }
      }
    });
  }

  // Unsubscribe from the geolocation watch using the initial ID
  stopTracking() {
    Geolocation.clearWatch({ id: this.watch }).then(() => {
      this.isTracking = false;
    });
  }

  //to upload destination data
  ambdest() {
    const data = {
      latitude:this.dest_latitude,
      longitude:this.dest_longitude
    };
    this.httpClient.put<any>('https://gps-app-cb9a0-default-rtdb.firebaseio.com/Users/'+this.user.uid+'/Destination.json', data)
    .subscribe(
      (res) => console.log(res),
      (err) => console.log(err)
    );
  }

  //to update data on firebase realtime database
  realtimeupdate(lat_x,longi_y){
    const data = {
      latitude:lat_x,
      longitude:longi_y
    };
    this.httpClient.put<any>('https://gps-app-cb9a0-default-rtdb.firebaseio.com/Users/'+this.user.uid+'/GPSdata.json', data)
    .subscribe(
      (res) => console.log(res),
      (err) => console.log(err)
    );

  }

  // for search bar
  ngOnInit() {
    //load Places Autocomplete
    this.mapsAPILoader.load().then(() => {
      //this.setCurrentLocation();
      this.geoCoder = new google.maps.Geocoder;

      let autocomplete = new google.maps.places.Autocomplete(this.searchElementRef.nativeElement, {
        types: ["address"]
      });
      autocomplete.addListener("place_changed", () => {
        this.ngZone.run(() => {
          //get the place result
          let place: google.maps.places.PlaceResult = autocomplete.getPlace();

          //verify result
          if (place.geometry === undefined || place.geometry === null) {
            return;
          }

          //set latitude, longitude, and address
          this.dest_latitude = place.geometry.location.lat();
          this.dest_longitude = place.geometry.location.lng();
          this.flag_dest = 1;
          this.ambdest();

          const latLng = new google.maps.LatLng(this.dest_latitude, this.dest_longitude);
          const mapOptions = {
            center: latLng,
            zoom: 13,
            mapTypeId: google.maps.MapTypeId.ROADMAP
          };

          this.mapdata = new google.maps.Map(this.mapElement.nativeElement, mapOptions);

          const marker = new google.maps.Marker({
            map: this.mapdata,
            animation: google.maps.Animation.DROP,
            position: latLng
          });
          this.markers.push(marker);

          this.getAddressdest(this.dest_latitude, this.dest_longitude);
        });
      });
    });
  }

  // to get the address of the source
  getAddressdest(latitude, longitude) {
    this.geoCoder.geocode({ 'location': { lat: latitude, lng: longitude } }, (results, status) => {
      console.log(results);
      console.log(status);
      if (status === 'OK') {
        if (results[0]) {
          this.dest_address = results[0].formatted_address;
        } else {
          window.alert('No results found');
        }
      } else {
        window.alert('Geocoder failed due to: ' + status);
      }

    });
  }

  // to get the address of the destination
  getAddresssource(latitude, longitude) {
    this.geoCoder.geocode({ 'location': { lat: latitude, lng: longitude } }, (results, status) => {
      console.log(results);
      console.log(status);
      if (status === 'OK') {
        if (results[0]) {
          this.source_address = results[0].formatted_address;
        } else {
          window.alert('No results found');
        }
      } else {
        window.alert('Geocoder failed due to: ' + status);
      }

    });
  }

  // to start route tracking
  routeTrackingStart() {
    this.flag_routing = 1;
    this.calculateAndDisplayRoute();
  }

  // to stop route tracking
  routeTrackingStop() {
    this.flag_routing = 0;
  }

  //for directions
  calculateAndDisplayRoute() {

    for (let index = 0; index < this.markers.length; index++) {
      this.markers[index].setMap(null);
    }
    this.markers = [];
    if (this.flag_source==1 && this.flag_dest==1) {

      const waypts = [];
      for (let i = 0; i < this.nodes_address.length; i++) {
        waypts.push({
          location: this.nodes_address[i],
          stopover: true,
        });
      }

      const that = this;

      this.directionsService.route({
        origin: this.source_address,
        destination: this.dest_address,
        waypoints: waypts,
        optimizeWaypoints: true,
        travelMode: 'DRIVING'
      }, (response, status) => {
        if (status === 'OK') {
          console.log(response);
          that.directionsDisplay.setDirections(response);
        } else {
          window.alert('Directions request failed due to ' + status);
        }
      });

    }

  }

  ngOnDestroy() {
    this.subscription.unsubscribe();
  }

  // to get the address of the nodes
  Getnodesaddress() {
    for (let i = 0; i < this.nodes_coords.length; i++) {

      this.geoCoder.geocode({ 'location': { lat: this.nodes_coords[i][0], lng: this.nodes_coords[i][1] } }, (results, status) => {
        console.log(results);
        console.log(status);
        if (status === 'OK') {
          if (results[0]) {
            this.nodes_address.push(results[0].formatted_address);
          } else {
            window.alert('No results found');
          }
        } else {
          window.alert('Geocoder failed due to: ' + status);
        }

      });

    }
  }

}
